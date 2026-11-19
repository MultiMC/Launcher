/* Copyright 2025-2026 Mehmet Samet Duman
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "JavaDownloadDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDir>
#include <QHeaderView>
#include <QSplitter>

#include "Application.h"
#include "BuildConfig.h"
#include "FileSystem.h"
#include "Json.h"
#include "net/Download.h"

JavaDownloadDialog::JavaDownloadDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Download Java"));
    setMinimumSize(700, 450);
    resize(800, 500);
    m_providers = JavaDownload::JavaProviderInfo::availableProviders();
    setupUi();
}

void JavaDownloadDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    // --- Three-column selection area ---
    auto *columnsLayout = new QHBoxLayout();

    // Left: Provider list
    auto *providerGroup = new QGroupBox(tr("Provider"), this);
    auto *providerLayout = new QVBoxLayout(providerGroup);
    m_providerList = new QListWidget(this);
    m_providerList->setIconSize(QSize(24, 24));
    for (const auto &provider : m_providers) {
        auto *item = new QListWidgetItem(APPLICATION->getThemedIcon(provider.iconName), provider.name);
        m_providerList->addItem(item);
    }
    providerLayout->addWidget(m_providerList);
    columnsLayout->addWidget(providerGroup, 1);

    // Center: Major version list
    auto *versionGroup = new QGroupBox(tr("Version"), this);
    auto *versionLayout = new QVBoxLayout(versionGroup);
    m_versionList = new QListWidget(this);
    versionLayout->addWidget(m_versionList);
    columnsLayout->addWidget(versionGroup, 1);

    // Right: Sub-version / build list
    auto *subVersionGroup = new QGroupBox(tr("Build"), this);
    auto *subVersionLayout = new QVBoxLayout(subVersionGroup);
    m_subVersionList = new QListWidget(this);
    subVersionLayout->addWidget(m_subVersionList);
    columnsLayout->addWidget(subVersionGroup, 1);

    mainLayout->addLayout(columnsLayout, 1);

    // Info label
    m_infoLabel = new QLabel(this);
    m_infoLabel->setWordWrap(true);
    m_infoLabel->setText(tr("Select a Java provider, version, and build to download."));
    mainLayout->addWidget(m_infoLabel);

    // Progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    m_progressBar->setRange(0, 100);
    mainLayout->addWidget(m_progressBar);

    // Status label
    m_statusLabel = new QLabel(this);
    m_statusLabel->setVisible(false);
    mainLayout->addWidget(m_statusLabel);

    // Buttons
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_downloadBtn = new QPushButton(tr("Download"), this);
    m_downloadBtn->setEnabled(false);
    buttonLayout->addWidget(m_downloadBtn);

    m_cancelBtn = new QPushButton(tr("Cancel"), this);
    buttonLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(m_providerList, &QListWidget::currentRowChanged,
            this, &JavaDownloadDialog::providerChanged);
    connect(m_versionList, &QListWidget::currentRowChanged,
            this, &JavaDownloadDialog::majorVersionChanged);
    connect(m_subVersionList, &QListWidget::currentRowChanged,
            this, &JavaDownloadDialog::subVersionChanged);
    connect(m_downloadBtn, &QPushButton::clicked, this, &JavaDownloadDialog::onDownloadClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &JavaDownloadDialog::onCancelClicked);

    // Select first provider
    if (m_providerList->count() > 0) {
        m_providerList->setCurrentRow(0);
    }
}

void JavaDownloadDialog::providerChanged(int index)
{
    if (index < 0 || index >= m_providers.size())
        return;

    m_versionList->clear();
    m_subVersionList->clear();
    m_downloadBtn->setEnabled(false);
    m_versions.clear();
    m_runtimes.clear();

    const auto &provider = m_providers[index];
    m_infoLabel->setText(tr("Loading versions for %1...").arg(provider.name));

    fetchVersionList(provider.uid);
}

void JavaDownloadDialog::majorVersionChanged(int index)
{
    if (index < 0 || index >= m_versions.size())
        return;

    m_subVersionList->clear();
    m_downloadBtn->setEnabled(false);
    m_runtimes.clear();

    const auto &version = m_versions[index];
    m_infoLabel->setText(tr("Loading builds for %1...").arg(version.versionId));

    fetchRuntimes(version.uid, version.versionId);
}

void JavaDownloadDialog::subVersionChanged(int index)
{
    if (index < 0 || index >= m_runtimes.size()) {
        m_downloadBtn->setEnabled(false);
        return;
    }

    const auto &rt = m_runtimes[index];
    m_infoLabel->setText(
        tr("Ready to download: %1\n"
           "Version: %2\n"
           "Platform: %3\n"
           "Checksum: %4")
        .arg(rt.name,
             rt.version.toString(),
             rt.runtimeOS,
             rt.checksumHash.isEmpty() ? tr("None") : tr("Yes (%1)").arg(rt.checksumType)));
    m_downloadBtn->setEnabled(true);
}

void JavaDownloadDialog::fetchVersionList(const QString &uid)
{
    m_fetchJob.reset();
    m_fetchData.clear();

    QString url = QString("%1%2/index.json").arg(BuildConfig.META_URL, uid);
    m_fetchJob = new NetJob(tr("Fetch Java versions"), APPLICATION->network());
    auto dl = Net::Download::makeByteArray(QUrl(url), &m_fetchData);
    m_fetchJob->addNetAction(dl);

    connect(m_fetchJob.get(), &NetJob::succeeded, this, [this, uid]() {
        m_fetchJob.reset();

        QJsonDocument doc;
        try {
            doc = Json::requireDocument(m_fetchData);
        } catch (const Exception &e) {
            m_infoLabel->setText(tr("Failed to parse version list: %1").arg(e.cause()));
            return;
        }
        if (!doc.isObject()) {
            m_infoLabel->setText(tr("Failed to parse version list."));
            return;
        }

        m_versions = JavaDownload::parseVersionIndex(doc.object(), uid);
        m_versionList->clear();

        for (const auto &ver : m_versions) {
            QString displayName = ver.versionId;
            if (displayName.startsWith("java")) {
                displayName = "Java " + displayName.mid(4);
            }
            m_versionList->addItem(displayName);
        }

        if (m_versions.size() > 0) {
            m_infoLabel->setText(tr("Select a version."));
        } else {
            m_infoLabel->setText(tr("No versions available for this provider."));
        }
    });

    connect(m_fetchJob.get(), &NetJob::failed, this, [this](QString reason) {
        m_fetchJob.reset();
        m_infoLabel->setText(tr("Failed to load versions: %1").arg(reason));
    });

    m_fetchJob->start();
}

void JavaDownloadDialog::fetchRuntimes(const QString &uid, const QString &versionId)
{
    m_fetchJob.reset();
    m_fetchData.clear();

    QString url = QString("%1%2/%3.json").arg(BuildConfig.META_URL, uid, versionId);
    m_fetchJob = new NetJob(tr("Fetch Java runtime details"), APPLICATION->network());
    auto dl = Net::Download::makeByteArray(QUrl(url), &m_fetchData);
    m_fetchJob->addNetAction(dl);

    connect(m_fetchJob.get(), &NetJob::succeeded, this, [this]() {
        m_fetchJob.reset();

        QJsonDocument doc;
        try {
            doc = Json::requireDocument(m_fetchData);
        } catch (const Exception &e) {
            m_infoLabel->setText(tr("Failed to parse runtime details: %1").arg(e.cause()));
            return;
        }
        if (!doc.isObject()) {
            m_infoLabel->setText(tr("Failed to parse runtime details."));
            return;
        }

        auto allRuntimes = JavaDownload::parseRuntimes(doc.object());
        QString myOS = JavaDownload::currentRuntimeOS();

        m_runtimes.clear();
        m_subVersionList->clear();
        for (const auto &rt : allRuntimes) {
            if (rt.runtimeOS == myOS) {
                m_runtimes.append(rt);
                m_subVersionList->addItem(rt.version.toString());
            }
        }

        if (m_runtimes.isEmpty()) {
            m_infoLabel->setText(tr("No builds available for your platform (%1).").arg(myOS));
            m_downloadBtn->setEnabled(false);
        } else {
            m_infoLabel->setText(tr("Select a build to download."));
            m_subVersionList->setCurrentRow(0);
        }
    });

    connect(m_fetchJob.get(), &NetJob::failed, this, [this](QString reason) {
        m_fetchJob.reset();
        m_infoLabel->setText(tr("Failed to load runtime details: %1").arg(reason));
    });

    m_fetchJob->start();
}

QString JavaDownloadDialog::javaInstallDir() const
{
    return FS::PathCombine(QDir::currentPath(), "java");
}

void JavaDownloadDialog::onDownloadClicked()
{
    int idx = m_subVersionList->currentRow();
    if (idx < 0 || idx >= m_runtimes.size())
        return;

    const auto &runtime = m_runtimes[idx];

    // Build target directory path: {dataPath}/java/{vendor}/{name}-{version}/
    QString dirName = QString("%1-%2").arg(runtime.name, runtime.version.toString());
    QString targetDir = FS::PathCombine(javaInstallDir(), runtime.vendor, dirName);

    // Check if already installed
    if (QDir(targetDir).exists()) {
        auto result = QMessageBox::question(this, tr("Already Installed"),
            tr("This Java version appears to be already installed at:\n%1\n\n"
               "Do you want to reinstall it?").arg(targetDir),
            QMessageBox::Yes | QMessageBox::No);
        if (result != QMessageBox::Yes) {
            return;
        }
        // Remove existing installation
        QDir(targetDir).removeRecursively();
    }

    m_downloadBtn->setEnabled(false);
    m_providerList->setEnabled(false);
    m_versionList->setEnabled(false);
    m_subVersionList->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_statusLabel->setVisible(true);

    m_downloadTask.reset(new JavaDownloadTask(runtime, targetDir, this));

    connect(m_downloadTask.get(), &Task::progress, this, [this](qint64 current, qint64 total) {
        if (total > 0) {
            m_progressBar->setValue(static_cast<int>(current * 100 / total));
        }
    });

    connect(m_downloadTask.get(), &Task::status, this, [this](const QString &status) {
        m_statusLabel->setText(status);
    });

    connect(m_downloadTask.get(), &Task::succeeded, this, [this]() {
        m_installedJavaPath = m_downloadTask->installedJavaPath();
        m_progressBar->setValue(100);
        m_statusLabel->setText(tr("Java installed successfully!"));

        QMessageBox::information(this, tr("Download Complete"),
            tr("Java has been downloaded and installed successfully.\n\n"
               "Java binary: %1").arg(m_installedJavaPath));

        accept();
    });

    connect(m_downloadTask.get(), &Task::failed, this, [this](const QString &reason) {
        m_progressBar->setVisible(false);
        m_statusLabel->setText(tr("Download failed: %1").arg(reason));
        m_downloadBtn->setEnabled(true);
        m_providerList->setEnabled(true);
        m_versionList->setEnabled(true);
        m_subVersionList->setEnabled(true);

        QMessageBox::warning(this, tr("Download Failed"),
            tr("Failed to download Java:\n%1").arg(reason));
    });

    m_downloadTask->start();
}

void JavaDownloadDialog::onCancelClicked()
{
    if (m_fetchJob) {
        m_fetchJob->abort();
        m_fetchJob.reset();
    }
    if (m_downloadTask) {
        m_downloadTask->abort();
        m_downloadTask.reset();
    }
    reject();
}
