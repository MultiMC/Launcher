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

#pragma once

#include <QMainWindow>
#include <memory>

#include "pages/BasePage.h"

#include "minecraft/auth/MojangAccountList.h"
#include "MultiMC.h"

namespace Ui
{
class AccountListPage;
}

class AuthenticateTask;

class AccountListPage : public QMainWindow, public BasePage
{
    Q_OBJECT
public:
    explicit AccountListPage(QWidget *parent = 0);
    ~AccountListPage();

    QString displayName() const override
    {
        return tr("Accounts");
    }
    QIcon icon() const override
    {
        auto icon = MMC->getThemedIcon("accounts");
        if(icon.isNull())
        {
            icon = MMC->getThemedIcon("noaccount");
        }
        return icon;
    }
    QString id() const override
    {
        return "accounts";
    }
    QString helpPage() const override
    {
        return "Getting-Started#adding-an-account";
    }

public slots:
    void on_actionAdd_triggered();
    void on_actionRemove_triggered();
    void on_actionSetDefault_triggered();
    void on_actionNoDefault_triggered();
    void on_actionUploadSkin_triggered();
    void on_actionDeleteSkin_triggered();

    void listChanged();

    //! Updates the states of the dialog's buttons.
    void updateButtonStates();

protected slots:
    void ShowContextMenu(const QPoint &pos);
    void addAccount(const QString& errMsg="");

private:
    void changeEvent(QEvent * event) override;
    QMenu * createPopupMenu() override;
    std::shared_ptr<MojangAccountList> m_accounts;
    Ui::AccountListPage *ui;
};
