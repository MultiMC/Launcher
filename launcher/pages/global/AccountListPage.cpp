/* Copyright 2013-2021 MultiMC Contributors
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
#include <QMenu>

#include <QDebug>

#include "net/NetJob.h"
#include "Env.h"

#include "dialogs/ProgressDialog.h"
#include "dialogs/LoginDialog.h"
#include "dialogs/CustomMessageBox.h"
#include "dialogs/SkinUploadDialog.h"
#include "tasks/Task.h"
#include "minecraft/auth/YggdrasilTask.h"
#include "minecraft/services/SkinDelete.h"

#include "MultiMC.h"

#include "BuildConfig.h"

AccountListPage::AccountListPage(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::AccountListPage)
{
    ui->setupUi(this);
    ui->listView->setEmptyString(tr(
        "Welcome!\n"
        "If you're new here, you can click the \"Add\" button to add your Mojang or Minecraft account."
    ));
    ui->listView->setEmptyMode(VersionListView::String);
    ui->listView->setContextMenuPolicy(Qt::CustomContextMenu);

    m_accounts = MMC->accounts();

    ui->listView->setModel(m_accounts.get());
    ui->listView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->listView->setSelectionMode(QAbstractItemView::SingleSelection);

    // Expand the account column
    ui->listView->header()->setSectionResizeMode(1, QHeaderView::Stretch);

    QItemSelectionModel *selectionModel = ui->listView->selectionModel();

    connect(selectionModel, &QItemSelectionModel::selectionChanged, [this](const QItemSelection &sel, const QItemSelection &dsel) {
        updateButtonStates();
    });
    connect(ui->listView, &VersionListView::customContextMenuRequested, this, &AccountListPage::ShowContextMenu);

    connect(m_accounts.get(), SIGNAL(listChanged()), SLOT(listChanged()));
    connect(m_accounts.get(), SIGNAL(activeAccountChanged()), SLOT(listChanged()));

    updateButtonStates();
}

AccountListPage::~AccountListPage()
{
    delete ui;
}

void AccountListPage::ShowContextMenu(const QPoint& pos)
{
    auto menu = ui->toolBar->createContextMenu(this, tr("Context menu"));
    menu->exec(ui->listView->mapToGlobal(pos));
    delete menu;
}

void AccountListPage::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }
    QMainWindow::changeEvent(event);
}

QMenu * AccountListPage::createPopupMenu()
{
    QMenu* filteredMenu = QMainWindow::createPopupMenu();
    filteredMenu->removeAction(ui->toolBar->toggleViewAction() );
    return filteredMenu;
}


void AccountListPage::listChanged()
{
    updateButtonStates();
}

void AccountListPage::on_actionAddMojang_triggered()
{
    MinecraftAccountPtr account = LoginDialog::newAccount(
        this,
        tr("Please enter your Mojang account email and password to add your account.")
    );

    if (account != nullptr)
    {
        m_accounts->addAccount(account);
        if (m_accounts->count() == 1)
            m_accounts->setActiveAccount(account->username());
    }
}

void AccountListPage::on_actionAddMicrosoft_triggered()
{
    QMessageBox msgBox;
    msgBox.setModal(true);
    msgBox.setWindowTitle("Not yet!");
    msgBox.setText("No Microsoft accounts yet. Work in progress!");
    msgBox.exec();
}

void AccountListPage::on_actionRemove_triggered()
{
    QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();
    if (selection.size() > 0)
    {
        QModelIndex selected = selection.first();
        m_accounts->removeAccount(selected);
    }
}

void AccountListPage::on_actionSetDefault_triggered()
{
    QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();
    if (selection.size() > 0)
    {
        QModelIndex selected = selection.first();
        MinecraftAccountPtr account =
            selected.data(AccountList::PointerRole).value<MinecraftAccountPtr>();
        m_accounts->setActiveAccount(account->username());
    }
}

void AccountListPage::on_actionNoDefault_triggered()
{
    m_accounts->setActiveAccount("");
}

void AccountListPage::updateButtonStates()
{
    // If there is no selection, disable buttons that require something selected.
    QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();

    ui->actionRemove->setEnabled(selection.size() > 0);
    ui->actionSetDefault->setEnabled(selection.size() > 0);
    ui->actionUploadSkin->setEnabled(selection.size() > 0);
    ui->actionDeleteSkin->setEnabled(selection.size() > 0);

    if(m_accounts->activeAccount().get() == nullptr) {
        ui->actionNoDefault->setEnabled(false);
        ui->actionNoDefault->setChecked(true);
    }
    else {
        ui->actionNoDefault->setEnabled(true);
        ui->actionNoDefault->setChecked(false);
    }

}

void AccountListPage::on_actionUploadSkin_triggered()
{
    QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();
    if (selection.size() > 0)
    {
        QModelIndex selected = selection.first();
        MinecraftAccountPtr account = selected.data(AccountList::PointerRole).value<MinecraftAccountPtr>();
        SkinUploadDialog dialog(account, this);
        dialog.exec();
    }
}

void AccountListPage::on_actionDeleteSkin_triggered()
{
    QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();
    if (selection.size() <= 0)
        return;

    QModelIndex selected = selection.first();
    AuthSessionPtr session = std::make_shared<AuthSession>();
    MinecraftAccountPtr account = selected.data(AccountList::PointerRole).value<MinecraftAccountPtr>();
    auto login = account->login(session);
    ProgressDialog prog(this);
    if (prog.execWithTask((Task*)login.get()) != QDialog::Accepted) {
        CustomMessageBox::selectable(this, tr("Skin Delete"), tr("Failed to login!"), QMessageBox::Warning)->exec();
        return;
    }
    auto deleteSkinTask = std::make_shared<SkinDelete>(this, session);
    if (prog.execWithTask((Task*)deleteSkinTask.get()) != QDialog::Accepted) {
        CustomMessageBox::selectable(this, tr("Skin Delete"), tr("Failed to delete current skin!"), QMessageBox::Warning)->exec();
        return;
    }
}
