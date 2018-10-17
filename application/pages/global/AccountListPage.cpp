/* Copyright 2013-2019 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "AccountListPage.h"
#include "ui_AccountListPage.h"

#include <QItemSelectionModel>

#include <QDebug>

#include "net/NetJob.h"
#include "net/URLConstants.h"
#include "Env.h"

#include "dialogs/ProgressDialog.h"
#include "dialogs/LoginDialog.h"
#include "dialogs/CustomMessageBox.h"
#include "dialogs/SkinUploadDialog.h"
#include "tasks/Task.h"
#include "minecraft/auth/YggdrasilTask.h"

#include "MultiMC.h"

enum class OfflineModeNameMode
{
    UseAccountName = 1,
    RememberPerAccount = 2,
    RememberPerInstance = 3,
    UseFixedName = 4
};

AccountListPage::AccountListPage(QWidget *parent)
    : QWidget(parent), ui(new Ui::AccountListPage)
{
    ui->setupUi(this);

    m_accounts = MMC->accounts();

    ui->listView->setModel(m_accounts.get());
    ui->listView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // Expand the account column
    ui->listView->header()->setSectionResizeMode(1, QHeaderView::Stretch);

    QItemSelectionModel *selectionModel = ui->listView->selectionModel();

    connect(selectionModel, &QItemSelectionModel::selectionChanged,
            [this](const QItemSelection &sel, const QItemSelection &dsel)
    { updateButtonStates(); });

    connect(m_accounts.get(), SIGNAL(listChanged()), SLOT(listChanged()));
    connect(m_accounts.get(), SIGNAL(activeAccountChanged()), SLOT(listChanged()));

    ui->offlineButtonGroup->setId(ui->useSelectedNameBtn, int(OfflineModeNameMode::UseAccountName));
    ui->offlineButtonGroup->setId(ui->rememberNamesForAccountsBtn, int(OfflineModeNameMode::RememberPerAccount));
    ui->offlineButtonGroup->setId(ui->rememberNamesForInstancesBtn, int(OfflineModeNameMode::RememberPerInstance));
    ui->offlineButtonGroup->setId(ui->useFixedNameBtn, int(OfflineModeNameMode::UseFixedName));

    connect(ui->offlineButtonGroup, SIGNAL(buttonToggled(int,bool)), this, SLOT(groupSelectionChanged(int,bool)));

    updateButtonStates();
    loadSettings();
}

AccountListPage::~AccountListPage()
{
    delete ui;
}

void AccountListPage::listChanged()
{
    updateButtonStates();
}

void AccountListPage::on_addAccountBtn_clicked()
{
    addAccount(tr("Please enter your Mojang or Minecraft account username and password to add "
                  "your account."));
}

void AccountListPage::on_rmAccountBtn_clicked()
{
    QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();
    if (selection.size() > 0)
    {
        QModelIndex selected = selection.first();
        m_accounts->removeAccount(selected);
    }
}

void AccountListPage::on_setDefaultBtn_clicked()
{
    QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();
    if (selection.size() > 0)
    {
        QModelIndex selected = selection.first();
        MojangAccountPtr account =
            selected.data(MojangAccountList::PointerRole).value<MojangAccountPtr>();
        m_accounts->setActiveAccount(account->username());
    }
}

void AccountListPage::on_noDefaultBtn_clicked()
{
    m_accounts->setActiveAccount("");
}

void AccountListPage::updateButtonStates()
{
    // If there is no selection, disable buttons that require something selected.
    QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();

    ui->rmAccountBtn->setEnabled(selection.size() > 0);
    ui->setDefaultBtn->setEnabled(selection.size() > 0);
    ui->uploadSkinBtn->setEnabled(selection.size() > 0);

    ui->noDefaultBtn->setDown(m_accounts->activeAccount().get() == nullptr);
}

void AccountListPage::addAccount(const QString &errMsg)
{
    // TODO: The login dialog isn't quite done yet
    MojangAccountPtr account = LoginDialog::newAccount(this, errMsg);

    if (account != nullptr)
    {
        m_accounts->addAccount(account);
        if (m_accounts->count() == 1)
            m_accounts->setActiveAccount(account->username());

        // Grab associated player skins
        auto job = new NetJob("Player skins: " + account->username());

        for (AccountProfile profile : account->profiles())
        {
            auto meta = Env::getInstance().metacache()->resolveEntry("skins", profile.id + ".png");
            auto action = Net::Download::makeCached(QUrl(URLConstants::SKINS_BASE + profile.id + ".png"), meta);
            job->addNetAction(action);
            meta->setStale(true);
        }

        job->start();
    }
}

void AccountListPage::on_uploadSkinBtn_clicked()
{
    QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();
    if (selection.size() > 0)
    {
        QModelIndex selected = selection.first();
        MojangAccountPtr account = selected.data(MojangAccountList::PointerRole).value<MojangAccountPtr>();
        SkinUploadDialog dialog(account, this);
        dialog.exec();
    }
}

bool AccountListPage::apply()
{
    applySettings();
    return true;
}

void AccountListPage::applySettings()
{
    auto s = MMC->settings();
    auto sortMode = (OfflineModeNameMode)ui->offlineButtonGroup->checkedId();
    switch (sortMode)
    {
    default:
    case OfflineModeNameMode::UseAccountName:
        s->set("OfflineModeNameMode", "UseAccountName");
        break;
    case OfflineModeNameMode::RememberPerAccount:
        s->set("OfflineModeNameMode", "RememberPerAccount");
        break;
    case OfflineModeNameMode::RememberPerInstance:
        s->set("OfflineModeNameMode", "RememberPerInstance");
        break;
    case OfflineModeNameMode::UseFixedName:
        s->set("OfflineModeNameMode", "UseFixedName");
        break;
    }
    s->set("OfflineModeName", ui->mainOfflineNameEdit->text());
}

void AccountListPage::loadSettings()
{
    auto s = MMC->settings();
    auto value = s->get("OfflineModeNameMode").toString();
    if(value == "UseAccountName")
    {
        ui->useSelectedNameBtn->setChecked(true);
    }
    else if(value == "RememberPerAccount")
    {
        ui->rememberNamesForAccountsBtn->setChecked(true);
    }
    else if(value == "RememberPerInstance")
    {
        ui->rememberNamesForInstancesBtn->setChecked(true);
    }
    else if(value == "UseFixedName")
    {
        ui->useFixedNameBtn->setChecked(true);
    }
    ui->mainOfflineNameEdit->setText(s->get("OfflineModeName").toString());
}

void AccountListPage::groupSelectionChanged(int, bool)
{
    auto sortMode = (OfflineModeNameMode)ui->offlineButtonGroup->checkedId();
    if(sortMode == OfflineModeNameMode::UseFixedName)
    {
        ui->mainOfflineNameEdit->setEnabled(true);
    }
    else
    {
        ui->mainOfflineNameEdit->setEnabled(false);
    }
}
