/* Copyright 2025 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include <QAction>
#include "ui/GuiUtil.h"
#include <QStatusBar>

#include "AccountsDialog.h"
#include "ui_AccountsDialog.h"

#include "Application.h"
#include "BuildConfig.h"
#include "DesktopServices.h"
#include "minecraft/auth/AccountTask.h"
#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"

#include "skins/CapeCache.h"
#include "skins/CapesModel.h"
#include "skins/SkinsModel.h"
#include "skins/SkinUtils.h"

#include <qrcode/QrCodeGenerator.h>
#include <QMessageBox>
#include <QClipboard>

constexpr auto selectionFlags = QItemSelectionModel::Clear | QItemSelectionModel::Select | QItemSelectionModel::Rows | QItemSelectionModel::Current;

AccountsDialog::AccountsDialog(QWidget *parent, const QString& internalId) : QDialog(parent), ui(new Ui::AccountsDialog)
{
    ui->setupUi(this);
    ui->windowLayout->setWindowFlags(Qt::Widget);
    m_statusBar = ui->windowLayout->statusBar();

    auto icon = APPLICATION->getThemedIcon("accounts");
    if(icon.isNull())
    {
        icon = APPLICATION->getThemedIcon("noaccount");
    }
    m_accounts = APPLICATION->accounts();
    ui->accountListView->setModel(m_accounts.get());
    ui->accountListView->setIconSize(QSize(48, 48));
    m_capesModel = new CapesModel(this);
    ui->capesView->setModel(m_capesModel);
    m_skinsModel = APPLICATION->skinsModel().get();
    connect(m_skinsModel, &SkinsModel::skinUpdated, this, &AccountsDialog::onSkinUpdated);
    connect(m_skinsModel, &SkinsModel::listUpdated, this, &AccountsDialog::onSkinModelUpdated);
    ui->skinsView->setModel(m_skinsModel);
    setWindowIcon(icon);
    setWindowTitle(tr("Minecraft Accounts"));

    QItemSelectionModel *skinSelectionModel = ui->skinsView->selectionModel();
    connect(skinSelectionModel, &QItemSelectionModel::selectionChanged, this, &AccountsDialog::onSkinSelectionChanged);
    QItemSelectionModel *capeSelectionModel = ui->capesView->selectionModel();
    connect(capeSelectionModel, &QItemSelectionModel::selectionChanged, this, &AccountsDialog::onCapeSelectionChanged);
    connect(ui->btnResetChanges, &QPushButton::clicked, this, &AccountsDialog::onRevertChangesClicked);
    connect(ui->btnApplyChanges, &QPushButton::clicked, this, &AccountsDialog::onApplyChangesClicked);
    connect(ui->saveSkinButton, &QPushButton::clicked, this, &AccountsDialog::onSaveSkinClicked);
    connect(ui->openSkinsButton, &QPushButton::clicked, this, &AccountsDialog::onOpenSkinsFolderClicked);

    connect(ui->signOutButton, &QPushButton::clicked, [&](bool on) {
        qDebug() << "Normal Sign Out button clicked";
        onSignOutButtonClicked(on);
    });
    connect(ui->signOutButton_Setup, &QPushButton::clicked, [&](bool on) {
        qDebug() << "Setup Sign Out button clicked";
        onSignOutButtonClicked(on);
    });
    connect(ui->signOutButton_Demo, &QPushButton::clicked, [&](bool on) {
        qDebug() << "Demo Sign Out button clicked";
        onSignOutButtonClicked(on);
    });
    connect(ui->signOutButton_Expired, &QPushButton::clicked, [&](bool on) {
        qDebug() << "Expired Sign Out button clicked";
        onSignOutButtonClicked(on);
    });

    connect(ui->removeAndSignInButton, &QPushButton::clicked, this, &AccountsDialog::onRemoveAndSignInButtonClicked);

    connect(ui->getMinecraftButton, &QPushButton::clicked, this, &AccountsDialog::onGetMinecraftButtonClicked);

    connect(ui->refreshButton, &QPushButton::clicked, this, &AccountsDialog::onRefreshButtonClicked);
    connect(ui->refreshButton_Setup, &QPushButton::clicked, this, &AccountsDialog::onRefreshButtonClicked);
    connect(ui->refreshButton_Demo, &QPushButton::clicked, this, &AccountsDialog::onRefreshButtonClicked);

    connect(ui->getFreshCodeButton, &QPushButton::clicked, this, &AccountsDialog::onGetFreshCodeButtonClicked);

    QItemSelectionModel *selectionModel = ui->accountListView->selectionModel();
    bool foundAccount = false;
    if(!internalId.isEmpty())
    {
        MinecraftAccountPtr account;
        int row;
        if(m_accounts->getAccountById(internalId, account, row))
        {
            selectionModel->select(m_accounts->index(row), selectionFlags);
            foundAccount = true;
        }
    }
    if(!foundAccount)
    {
        if(m_accounts->count() == 1)
        {
            selectionModel->select(m_accounts->index(0), selectionFlags);
        }
        else
        {
            if(m_accounts->defaultAccount())
            {
                selectionModel->select(m_accounts->defaultAccountIndex(), selectionFlags);
            }
            else
            {
                selectionModel->select(m_accounts->index(1), selectionFlags);
            }
        }
    }
    updateStates();

    connect(selectionModel, &QItemSelectionModel::selectionChanged, [this](const QItemSelection &sel, const QItemSelection &dsel) {
        updateStates();
    });
    connect(m_accounts.get(), &AccountList::accountActivityChanged, this, &AccountsDialog::onAccountActivityChanged);
    connect(m_accounts.get(), &AccountList::accountChanged, this, &AccountsDialog::onAccountChanged);

    connect(ui->linkButton, &QToolButton::clicked, this, &AccountsDialog::onQrButtonClicked);
    connect(ui->copyLinkButton, &QToolButton::clicked, this, &AccountsDialog::onCopyLinkButtonClicked);
    connect(&m_externalLoginTimer, &QTimer::timeout, this, &AccountsDialog::externalLoginTick);

    ui->createProfileErrorLabel->setVisible(false);

    connect(ui->modelButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(onModelRadioClicked(QAbstractButton*)));

    // Profile creation elements
    m_goodIcon = APPLICATION->getThemedIcon("status-good");
    m_yellowIcon = APPLICATION->getThemedIcon("status-yellow");
    m_badIcon = APPLICATION->getThemedIcon("status-bad");
    QRegExp permittedNames("[a-zA-Z0-9_]{3,16}");
    auto nameEdit = ui->createProfileNameEdit;
    nameEdit->setValidator(new QRegExpValidator(permittedNames));
    nameEdit->setClearButtonEnabled(true);

    m_validityAction = nameEdit->addAction(m_yellowIcon, QLineEdit::LeadingPosition);
    connect(nameEdit, &QLineEdit::textEdited, this, &AccountsDialog::onNameEdited);

    m_checkStartTimer.setSingleShot(true);
    connect(&m_checkStartTimer, &QTimer::timeout, this, &AccountsDialog::onNameCheckTimerTriggered);

    connect(ui->createProfileButton, &QCommandLinkButton::clicked, this, &AccountsDialog::onCreateProfileButtonClicked);

    setNameStatus(NameStatus::NotSet, QString());

    GuiUtil::restoreDialogGeometry(this, "AccountsDialogGeometry");
    ui->splitter->restoreState(QByteArray::fromBase64(APPLICATION->settings()->get("AccountsDialogSplitterState").toByteArray()));
}

AccountsDialog::~AccountsDialog()
{
    delete ui;
}

void AccountsDialog::closeEvent(QCloseEvent* event)
{
    APPLICATION->settings()->set("AccountsDialogSplitterState", ui->splitter->saveState().toBase64());
    GuiUtil::saveDialogGeometry(this, "AccountsDialogGeometry");
    QDialog::closeEvent(event);
}


void AccountsDialog::onRevertChangesClicked(bool)
{
    revertEdits();
}

void AccountsDialog::onOpenSkinsFolderClicked(bool)
{
    DesktopServices::openDirectory(m_skinsModel->path());
}


void AccountsDialog::onSaveSkinClicked(bool)
{
    if(!m_currentAccount || !m_currentAccount->hasProfile())
    {
        return;
    }
    auto skin = effectiveSkin().getTextureDataFor(effectiveModel());
    QModelIndex index = m_skinsModel->installSkin(skin, m_currentAccount->profileName());
    if(index.isValid())
    {
        ui->saveSkinButton->setEnabled(false);
    }
}


void AccountsDialog::onApplyChangesClicked(bool)
{
    if(!m_currentAccount || !m_currentAccount->hasProfile())
    {
        return;
    }
    if(!m_skinEdit)
    {
        return;
    }
    auto task = m_currentAccount->setSkin(m_skinEdit->model, m_skinEdit->skinEntry.getTextureDataFor(m_skinEdit->model), m_skinEdit->cape);
    if(task)
    {
        auto account = m_currentAccount;
        connect(task.get(), &AccountTask::apiError, [this, account](const MojangError& error ) {
            if(m_currentAccount == account)
            {
                QMessageBox::critical(this, tr("Failed to apply skin"), error.toString());
            }
        });
        connect(task.get(), &AccountTask::succeeded, [this, account]() {
            if(m_currentAccount == account)
            {
                revertEdits();
            }
        });
        task->start();
    }
}

void AccountsDialog::revertEdits()
{
    ui->skinsView->clearSelection();
    ui->capesView->clearSelection();

    m_skinEdit = nonstd::nullopt;
    ui->btnApplyChanges->setEnabled(false);
    ui->btnResetChanges->setEnabled(false);
    updateSkinDisplay();
}

bool AccountsDialog::startSkinEdit()
{
    if(m_skinEdit)
    {
        return true;
    }
    if(!m_currentAccount || !m_currentAccount->hasProfile())
    {
        return false;
    }

    ui->btnApplyChanges->setEnabled(true);
    ui->btnResetChanges->setEnabled(true);
    m_skinEdit = m_playerSkinState;
    return true;
}

Skins::Model AccountsDialog::effectiveModel() const
{
    if(m_skinEdit)
        return m_skinEdit->model;
    return m_playerSkinState.model;
}

const QString & AccountsDialog::effectiveCape() const
{
    if(m_skinEdit)
        return m_skinEdit->cape;
    return m_playerSkinState.cape;
}

const Skins::SkinEntry & AccountsDialog::effectiveSkin() const
{
    if(m_skinEdit)
        return m_skinEdit->skinEntry;
    return m_playerSkinState.skinEntry;
}


void AccountsDialog::editModel(Skins::Model model)
{
    if(!startSkinEdit())
        return;
    m_skinEdit->model = model;
    updateSkinDisplay();
}

void AccountsDialog::editCape(const QString& cape)
{
    if(!startSkinEdit())
        return;
    m_skinEdit->cape = cape;
    updateSkinDisplay();
}


void AccountsDialog::editSkin(const Skins::SkinEntry& newEntry)
{
    if(!startSkinEdit())
        return;

    m_skinEdit->skinEntry = newEntry;
    updateSkinDisplay();
}

void AccountsDialog::onSkinSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    if(!m_currentAccount || !m_currentAccount->hasProfile())
    {
        return;
    }
    const auto& indexes = selected.indexes();
    if(indexes.size() == 0)
    {
        return;
    }
    const auto& index = indexes[0];
    int row = index.row();
    editSkin(m_skinsModel->at(row));
}

void AccountsDialog::onCapeSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    if(!m_currentAccount || !m_currentAccount->hasProfile())
    {
        return;
    }
    const auto& indexes = selected.indexes();
    if(indexes.size() == 0)
    {
        return;
    }
    const auto& index = indexes[0];
    int row = index.row();
    editCape(m_capesModel->at(row));
}

void AccountsDialog::onSkinUpdated(const QString& key)
{
    if(effectiveSkin().name == key)
    {
        editSkin(m_skinsModel->skinEntry(key));
    }
}

void AccountsDialog::onSkinModelUpdated()
{
    auto& skin = effectiveSkin();
    auto model = effectiveModel();

    auto textureID = skin.getTextureIDFor(model);
    ui->saveSkinButton->setEnabled(m_skinsModel->skinEntryByTextureID(textureID).isNull());
}


void AccountsDialog::onAccountChanged(MinecraftAccount* account)
{
    if(m_currentAccount.get() == account)
    {
        updateStates();
    }
}

void AccountsDialog::onAccountActivityChanged(MinecraftAccount* account, bool active)
{
    if(m_currentAccount.get() == account)
    {
        updateStates();
    }
}

void AccountsDialog::updateStates()
{
    // If there is no selection, disable buttons that require something selected.
    QModelIndexList selection = ui->accountListView->selectionModel()->selectedIndexes();
    bool hasSelection = selection.size() > 0;
    bool accountIsReady = false;
    auto prevAccount = m_currentAccount;
    m_currentAccount = nullptr;
    if (hasSelection)
    {
        QModelIndex selected = selection.first();
        m_currentAccount = selected.data(AccountList::PointerRole).value<MinecraftAccountPtr>();
        accountIsReady = m_currentAccount && !m_currentAccount->isActive();
    }

    // New account page
    if(!m_currentAccount)
    {
        ui->accountPageStack->setCurrentWidget(ui->loginPage);

        // Setup the login task and start it
        if(!m_loginAccount)
        {
            startLogin();
        }
        return;
    }

    if(m_currentAccount->accountState() == AccountState::Expired)
    {
        ui->accountPageStack->setCurrentWidget(ui->expiredPage);
        if(m_currentAccount->hasProfile())
        {
            ui->selectedAccountLabel_Expired->setText(m_currentAccount->profileName());
            ui->selectedAccountIconLabel_Expired->setIcon(m_currentAccount->getFace());
        }
        else
        {
            ui->selectedAccountLabel_Expired->setText(m_currentAccount->gamerTag());
            ui->selectedAccountIconLabel_Expired->setIcon(APPLICATION->getThemedIcon("noaccount"));
        }
        return;
    }

    // Demo account (no Minecraft entitlement)
    if(!m_currentAccount->ownsMinecraft())
    {
        ui->accountPageStack->setCurrentWidget(ui->demoPage);
        ui->setupProfilePage->setEnabled(accountIsReady);
        ui->selectedAccountLabel_Demo->setText(m_currentAccount->gamerTag());
        ui->selectedAccountIconLabel_Demo->setIcon(APPLICATION->getThemedIcon("noaccount"));
        return;
    }

    // Profile setup page
    if(!m_currentAccount->hasProfile())
    {
        ui->accountPageStack->setCurrentWidget(ui->setupProfilePage);
        ui->setupProfilePage->setEnabled(accountIsReady);
        ui->selectedAccountLabel_Setup->setText(m_currentAccount->gamerTag());
        ui->selectedAccountIconLabel_Setup->setIcon(APPLICATION->getThemedIcon("noaccount"));
        return;
    }

    // Full account page
    ui->fullAccountPage->setEnabled(accountIsReady);
    if(prevAccount != m_currentAccount)
    {
        revertEdits();
    }

    m_capesModel->setAccount(m_currentAccount);
    ui->accountPageStack->setCurrentWidget(ui->fullAccountPage);
    ui->selectedAccountLabel->setText(m_currentAccount->profileName());
    ui->selectedAccountIconLabel->setIcon(m_currentAccount->getFace());

    QByteArray playerSkinData = m_currentAccount->getSkin();
    QImage image;
    QString textureID;
    Skins::readSkinFromData(playerSkinData, image, textureID);
    auto maybeEntry = m_skinsModel->skinEntryByTextureID(textureID);
    m_playerSkinState = SkinState{
        m_currentAccount->getCurrentCape(),
        m_currentAccount->getSkinModel(),
        (!maybeEntry.isNull()) ? maybeEntry : Skins::SkinEntry("player", "", image, textureID, playerSkinData)
    };

    updateSkinDisplay();
}

void AccountsDialog::updateSkinDisplay()
{
    QImage capeImage;
    auto cape = effectiveCape();
    if(!cape.isEmpty())
    {
        auto capeCache = APPLICATION->capeCache();
        capeImage = capeCache->getCapeImage(cape);
    }
    auto& skin = effectiveSkin();
    auto model = effectiveModel();

    auto textureID = skin.getTextureIDFor(model);
    ui->saveSkinButton->setEnabled(m_skinsModel->skinEntryByTextureID(textureID).isNull());

    ui->skinPreviewWidget->setAll(model, skin.getTextureFor(model), capeImage);
    {
        const QSignalBlocker blocker(ui->modelButtonGroup);
        switch(model)
        {
            case Skins::Model::Classic:
                ui->radioClassic->setChecked(true);
                break;
            case Skins::Model::Slim:
                ui->radioSlim->setChecked(true);
                break;
        }
    }
}


void AccountsDialog::stopLogin()
{
    m_externalLoginTimer.stop();
    m_loginTask = nullptr;
    m_loginAccount = nullptr;
    ui->getFreshCodeButton->setEnabled(true);
    ui->linkButton->setVisible(false);
    ui->copyLinkButton->setVisible(false);
}

void AccountsDialog::startLogin()
{
    ui->getFreshCodeButton->setEnabled(false);
    ui->linkButton->setVisible(false);
    ui->copyLinkButton->setVisible(false);
    m_loginAccount = MinecraftAccount::createBlankMSA();
    m_loginTask = m_loginAccount->loginMSA();
    connect(m_loginTask.get(), &Task::failed, this, &AccountsDialog::onLoginTaskFailed);
    connect(m_loginTask.get(), &Task::succeeded, this, &AccountsDialog::onLoginTaskSucceeded);
    connect(m_loginTask.get(), &Task::status, this, &AccountsDialog::onLoginTaskStatus);
    connect(m_loginTask.get(), &Task::progress, this, &AccountsDialog::onLoginTaskProgress);
    connect(m_loginTask.get(), &AccountTask::showVerificationUriAndCode, this, &AccountsDialog::showVerificationUriAndCode);
    connect(m_loginTask.get(), &AccountTask::hideVerificationUriAndCode, this, &AccountsDialog::hideVerificationUriAndCode);
    m_loginTask->start();
}

void AccountsDialog::onGetFreshCodeButtonClicked(bool)
{
    stopLogin();
    startLogin();
}

void AccountsDialog::onQrButtonClicked(bool)
{
    DesktopServices::openUrl(m_codeUrl);
}

void AccountsDialog::onCopyLinkButtonClicked(bool)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(m_codeUrl.toString());
}

void AccountsDialog::externalLoginTick() {
    m_externalLoginElapsed++;
    ui->progressBar->setValue(m_externalLoginTimeout - m_externalLoginElapsed);
    ui->progressBar->repaint();
    if(m_externalLoginElapsed == 5)
    {
        ui->getFreshCodeButton->setEnabled(true);
    }

    if(m_externalLoginElapsed >= m_externalLoginTimeout) {
        stopLogin();

    }
}

void AccountsDialog::onCapeUpdated(const QString& uuid)
{
    auto currentCape = effectiveCape();
    if(uuid != currentCape)
    {
        return;
    }
    // we need to update the cape image in the preview widget
    auto capeCache = APPLICATION->capeCache();
    auto capeImage = capeCache->getCapeImage(currentCape);
    ui->skinPreviewWidget->setCapeImage(capeImage);
}

void AccountsDialog::onModelRadioClicked(QAbstractButton* radio)
{
    Skins::Model model = (radio == ui->radioClassic) ? Skins::Model::Classic : Skins::Model::Slim;
    editModel(model);
}


void AccountsDialog::showVerificationUriAndCode(const QUrl& uri, const QString& code, int expiresIn) {
    m_externalLoginElapsed = 0;
    m_externalLoginTimeout = expiresIn;

    m_externalLoginTimer.setInterval(1000);
    m_externalLoginTimer.setSingleShot(false);
    m_externalLoginTimer.start();

    ui->progressBar->setMaximum(expiresIn);
    ui->progressBar->setValue(m_externalLoginTimeout - m_externalLoginElapsed);
    ui->progressBar->setVisible(true);

    m_codeUrl = uri;
    QUrlQuery query;
    query.addQueryItem("otc", code);
    m_codeUrl.setQuery(query);
    QString codeUrlString = m_codeUrl.toString();

    QImage qrcode = qrcode::generateQr(codeUrlString, 300);
    ui->linkButton->setIcon(QPixmap::fromImage(qrcode));
    ui->linkButton->setText(codeUrlString);
    ui->linkButton->setVisible(true);
    ui->copyLinkButton->setVisible(true);

    ui->label->setText(tr("You can scan the QR code and complete the login process on a separate device, or you can open the link and login on this machine."));
    m_code = code;
}

void AccountsDialog::hideVerificationUriAndCode() {
    ui->linkButton->setVisible(false);
    ui->copyLinkButton->setVisible(false);
    ui->progressBar->setVisible(false);
    m_externalLoginTimer.stop();
}

void AccountsDialog::onLoginTaskFailed(const QString &reason)
{
    // Set message
    auto lines = reason.split('\n');
    QString processed;
    for(auto line: lines) {
        if(line.size()) {
            processed += "<font color='red'>" + line + "</font><br />";
        }
        else {
            processed += "<br />";
        }
    }
    ui->label->setText(processed);

    ui->progressBar->setVisible(false);
}

void AccountsDialog::onLoginTaskSucceeded()
{
    m_loginTask = nullptr;
    QModelIndex index = m_accounts->addAccount(m_loginAccount);

    ui->accountListView->selectionModel()->select(index, selectionFlags);
    if(!m_accounts->defaultAccount())
    {
        m_accounts->setData(index, Qt::Checked, Qt::CheckStateRole);
    }
    m_loginAccount = nullptr;
}

void AccountsDialog::onLoginTaskStatus(const QString &status)
{
    ui->label->setText(status);
}

void AccountsDialog::onLoginTaskProgress(qint64 current, qint64 total)
{
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(current);
}

void AccountsDialog::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}

void AccountsDialog::onRefreshButtonClicked(bool)
{
    if(m_currentAccount)
    {
        m_accounts->requestRefresh(m_currentAccount->internalId());
    }
}

void AccountsDialog::onSignOutButtonClicked(bool)
{
    if(m_currentAccount)
    {
        m_accounts->removeAccount(m_currentAccount->internalId());
    }
}

void AccountsDialog::onRemoveAndSignInButtonClicked(bool)
{
    if(m_currentAccount)
    {
        m_accounts->removeAccount(m_currentAccount->internalId());
    }
    ui->accountListView->setCurrentIndex(m_accounts->index(0));
}

void AccountsDialog::onGetMinecraftButtonClicked(bool)
{
    DesktopServices::openUrl(QUrl("https://www.minecraft.net/en-us/store/minecraft-java-bedrock-edition-pc"));
}


void AccountsDialog::setNameStatus(AccountsDialog::NameStatus status, QString errorString = QString())
{
    nameStatus = status;
    auto okButton = ui->createProfileButton;
    switch(nameStatus)
    {
        case NameStatus::Available: {
            m_validityAction->setIcon(m_goodIcon);
            okButton->setEnabled(true);
        }
        break;
        case NameStatus::NotSet:
        case NameStatus::Pending:
            m_validityAction->setIcon(m_yellowIcon);
            okButton->setEnabled(false);
            break;
        case NameStatus::Exists:
        case NameStatus::Error:
            m_validityAction->setIcon(m_badIcon);
            okButton->setEnabled(false);
            break;
    }
    if(!errorString.isEmpty()) {
        ui->createProfileErrorLabel->setText(errorString);
        ui->createProfileErrorLabel->setVisible(true);
    }
    else {
        ui->createProfileErrorLabel->setVisible(false);
    }
}

void AccountsDialog::onNameEdited(const QString& name)
{
    if(!ui->createProfileNameEdit->hasAcceptableInput()) {
        setNameStatus(NameStatus::NotSet, tr("Name is too short - must be between 3 and 16 characters long."));
        return;
    }
    scheduleCheck(name);
}

void AccountsDialog::scheduleCheck(const QString& name) {
    m_queuedCheck = name;
    setNameStatus(NameStatus::Pending);
    m_checkStartTimer.start(1000);
}

void AccountsDialog::onNameCheckTimerTriggered() {
    if(m_isChecking) {
        return;
    }
    if(m_queuedCheck.isNull()) {
        return;
    }
    checkName(m_queuedCheck);
}

void AccountsDialog::checkName(const QString &name) {
    if(m_isChecking) {
        return;
    }

    m_currentCheck = name;
    m_isChecking = true;

    auto token = m_currentAccount->accessToken();

    auto url = QString("%1/minecraft/profile/name/%2/available").arg(BuildConfig.API_BASE).arg(name);
    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());

    AuthRequest *requestor = new AuthRequest(this);
    connect(requestor, &AuthRequest::finished, this, &AccountsDialog::onNameCheckFinished);
    requestor->get(request);
}

void AccountsDialog::onNameCheckFinished(
    QNetworkReply::NetworkError error,
    QByteArray data,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    auto requestor = qobject_cast<AuthRequest *>(QObject::sender());
    requestor->deleteLater();

    if(error == QNetworkReply::NoError) {
        auto doc = QJsonDocument::fromJson(data);
        auto root = doc.object();
        auto statusValue = root.value("status").toString("INVALID");
        if(statusValue == "AVAILABLE") {
            setNameStatus(NameStatus::Available);
        }
        else if (statusValue == "DUPLICATE") {
            setNameStatus(NameStatus::Exists, tr("Minecraft profile with name %1 already exists.").arg(m_currentCheck));
        }
        else if (statusValue == "NOT_ALLOWED") {
            setNameStatus(NameStatus::Exists, tr("The name %1 is not allowed.").arg(m_currentCheck));
        }
        else {
            setNameStatus(NameStatus::Error, tr("Unhandled profile name status: %1").arg(statusValue));
        }
    }
    else {
        setNameStatus(NameStatus::Error, tr("Failed to check name availability."));
    }
    m_isChecking = false;
}

void AccountsDialog::onCreateProfileButtonClicked(bool)
{
    auto task = m_currentAccount->createMinecraftProfile(ui->createProfileNameEdit->text());
    if(task)
    {
        connect(task.get(), &AccountTask::apiError, this, &AccountsDialog::onProfileCreationError);
        task->start();
    }
}

void AccountsDialog::onProfileCreationError(const MojangError& error)
{
    ui->createProfileErrorLabel->setVisible(true);
    if(error.jsonParsed)
    {
        ui->createProfileErrorLabel->setText(error.errorMessage);
    }
    else
    {
        ui->createProfileErrorLabel->setText(error.toString());
    }
}
