#include "CFPage.h"
#include "ui_CFPage.h"

#include <QKeyEvent>

#include "Application.h"
#include "Json.h"
#include "ui/dialogs/NewInstanceDialog.h"
#include "InstanceImportTask.h"
#include "CFModel.h"
#include "../DescriptionDocument.h"

CFPage::CFPage(NewInstanceDialog* dialog, QWidget *parent)
    : QWidget(parent), ui(new Ui::CFPage), dialog(dialog)
{
    ui->setupUi(this);
    connect(ui->searchButton, &QPushButton::clicked, this, &CFPage::triggerSearch);
    ui->searchEdit->installEventFilter(this);
    listModel = new CurseForge::ListModel(this);
    ui->packView->setModel(listModel);

    ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    // index is used to set the sorting with the curseforge api
    ui->sortByBox->addItem(tr("Sort by featured"));
    ui->sortByBox->addItem(tr("Sort by popularity"));
    ui->sortByBox->addItem(tr("Sort by last updated"));
    ui->sortByBox->addItem(tr("Sort by name"));
    ui->sortByBox->addItem(tr("Sort by author"));
    ui->sortByBox->addItem(tr("Sort by total downloads"));

    connect(ui->sortByBox, SIGNAL(currentIndexChanged(int)), this, SLOT(triggerSearch()));
    connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &CFPage::onSelectionChanged);
    connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &CFPage::onVersionSelectionChanged);
}

CFPage::~CFPage()
{
    delete ui;
}

bool CFPage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->searchEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return) {
            triggerSearch();
            keyEvent->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

bool CFPage::shouldDisplay() const
{
    return true;
}

void CFPage::openedImpl()
{
    suggestCurrent();
    triggerSearch();
}

void CFPage::triggerSearch()
{
    listModel->searchWithTerm(ui->searchEdit->text(), ui->sortByBox->currentIndex());
}

void CFPage::refreshRightPane() {
    QString text = "";
    QString name = m_current.name;

    if (m_current.websiteUrl.isEmpty())
        text = name;
    else
        text = "<a href=\"" + m_current.websiteUrl + "\">" + name + "</a>";
    if (!m_current.wikiUrl.isEmpty()) {
        text += "<br>" + tr("<a href=\"%1\">Wiki</a>").arg(m_current.wikiUrl);
    }
    if (!m_current.issuesUrl.isEmpty()) {
        text += "<br>" + tr("<a href=\"%1\">Bug tracker</a>").arg(m_current.issuesUrl);
    }
    if (!m_current.sourceUrl.isEmpty()) {
        text += "<br>" + tr("<a href=\"%1\">Source code</a>").arg(m_current.sourceUrl);
    }
    if (!m_current.authors.empty()) {
        auto authorToStr = [](CurseForge::ModpackAuthor & author) {
            if(author.url.isEmpty()) {
                return author.name;
            }
            return QString("<a href=\"%1\">%2</a>").arg(author.url, author.name);
        };
        QStringList authorStrs;
        for(auto & author: m_current.authors) {
            authorStrs.push_back(authorToStr(author));
        }
        text += "<br>" + tr(" by ") + authorStrs.join(", ");
    }
    text += "<br><br>";

    if(!m_current.description.isEmpty()) {
        text += m_current.description;
    }
    else {
        text += m_current.summary;
    }

    auto document = new Modplatform::DescriptionDocument(text);
    connect(document, &Modplatform::DescriptionDocument::layoutUpdateRequired, this, &CFPage::forceDocumentLayout);
    ui->packDescription->setDocument(document);
}


void CFPage::onSelectionChanged(QModelIndex first, QModelIndex second)
{
    ui->versionSelectionBox->clear();

    if(!first.isValid())
    {
        if(isOpened)
        {
            dialog->setSuggestedPack();
        }
        return;
    }

    m_current = listModel->data(first, Qt::UserRole).value<CurseForge::IndexedPack>();
    refreshRightPane();

    if (m_current.descriptionLoaded == false) {
        qDebug() << "Loading curseforge modpack description";
        NetJob *netJob = new NetJob(QString("CurseForge::PackDescription(%1)").arg(m_current.name), APPLICATION->network());
        std::shared_ptr<QByteArray> response = std::make_shared<QByteArray>();
        int addonId = m_current.addonId;
        auto download = Net::Download::makeByteArray(QString("https://api.curseforge.com/v1/mods/%1/description").arg(addonId), response.get());
        download->setExtraHeader("x-api-key", APPLICATION->curseAPIKey());
        netJob->addNetAction(download);
        QObject::connect(netJob, &NetJob::succeeded, this, [this, response, addonId]
        {
            if (addonId != m_current.addonId) {
                return;
            }
            QJsonParseError parse_error;
            QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
            if(parse_error.error != QJsonParseError::NoError) {
                qWarning() << "Error while parsing JSON response from CurseForge at " << parse_error.offset << " reason: " << parse_error.errorString();
                qWarning() << *response;
                return;
            }
            QString description = Json::ensureString(doc.object(), "data");
            m_current.description = description;
            refreshRightPane();
        });
        netJob->start();
    }
    if (m_current.versionsLoaded == false)
    {
        qDebug() << "Loading curseforge modpack versions";
        NetJob *netJob = new NetJob(QString("CurseForge::PackVersions(%1)").arg(m_current.name), APPLICATION->network());
        std::shared_ptr<QByteArray> response = std::make_shared<QByteArray>();
        int addonId = m_current.addonId;
        auto download = Net::Download::makeByteArray(QString("https://api.curseforge.com/v1/mods/%1/files").arg(addonId), response.get());
        download->setExtraHeader("x-api-key", APPLICATION->curseAPIKey());
        netJob->addNetAction(download);

        QObject::connect(netJob, &NetJob::succeeded, this, [this, response, addonId]
        {
            if (addonId != m_current.addonId) {
                return;
            }
            QJsonParseError parse_error;
            QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
            if(parse_error.error != QJsonParseError::NoError) {
                qWarning() << "Error while parsing JSON response from CurseForge at " << parse_error.offset << " reason: " << parse_error.errorString();
                qWarning() << *response;
                return;
            }
            QJsonArray arr = Json::ensureArray(doc.object(), "data");
            try
            {
                CurseForge::loadIndexedPackVersions(m_current, arr);
            }
            catch(const JSONValidationError &e)
            {
                qDebug() << *response;
                qWarning() << "Error while reading curseforge modpack version: " << e.cause();
            }

            for(auto version : m_current.versions) {
                ui->versionSelectionBox->addItem(version.version, QVariant(version.downloadUrl));
            }

            suggestCurrent();
        });
        netJob->start();
    }
    else
    {
        for(auto version : m_current.versions) {
            ui->versionSelectionBox->addItem(version.version, QVariant(version.downloadUrl));
        }

        suggestCurrent();
    }
}

void CFPage::suggestCurrent()
{
    if(!isOpened)
    {
        return;
    }

    if (selectedVersion.isEmpty())
    {
        dialog->setSuggestedPack();
        return;
    }

    dialog->setSuggestedPack(m_current.name, new InstanceImportTask(selectedVersion));
    QString editedLogoName;
    editedLogoName = "curseforge_" + m_current.logoName.section(".", 0, 0);
    listModel->getLogo(m_current.logoName, m_current.logoUrl, [this, editedLogoName](QString logo)
    {
        dialog->setSuggestedIconFromFile(logo, editedLogoName);
    });
}

void CFPage::onVersionSelectionChanged(QString data)
{
    if(data.isNull() || data.isEmpty())
    {
        selectedVersion = "";
        return;
    }
    selectedVersion = ui->versionSelectionBox->currentData().toString();
    suggestCurrent();
}

void CFPage::forceDocumentLayout() {
    ui->packDescription->document()->adjustSize();
}
