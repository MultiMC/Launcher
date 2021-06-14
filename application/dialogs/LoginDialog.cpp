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

#include "LoginDialog.h"
#include "ui_LoginDialog.h"
#include "minecraft/auth/AuthProviders.h"
#include "minecraft/auth/YggdrasilTask.h"

#include <QtWidgets/QPushButton>

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent), ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    ui->progressBar->setVisible(false);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    for(auto provider: AuthProviders::getAll()) {
        QRadioButton *button = new QRadioButton(provider->displayName());
        m_radioButtons[provider->id()] = button;
        ui->radioLayout->addWidget(button);
    }
    m_radioButtons["dummy"]->setChecked(true);
    adjustSize();

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

// Stage 1: User interaction
void LoginDialog::accept()
{
    setUserInputsEnabled(false);
    ui->progressBar->setVisible(true);

    m_account = Account::createFromUsername(ui->userTextBox->text());
    for(auto providerId: m_radioButtons.keys()){
        if(m_radioButtons[providerId]->isChecked()) {
            m_account->setProvider(AuthProviders::lookup(providerId));
            break;
        }
    }

    // Setup the login task and start it
    m_loginTask = m_account->login(nullptr, ui->passTextBox->text());
    connect(m_loginTask.get(), &Task::failed, this, &LoginDialog::onTaskFailed);
    connect(m_loginTask.get(), &Task::succeeded, this,
            &LoginDialog::onTaskSucceeded);
    connect(m_loginTask.get(), &Task::status, this, &LoginDialog::onTaskStatus);
    connect(m_loginTask.get(), &Task::progress, this, &LoginDialog::onTaskProgress);
    if (!m_loginTask)
    {
        onTaskSucceeded();
    } else {
        m_loginTask->start();
    }
}

void LoginDialog::setUserInputsEnabled(bool enable)
{
    ui->userTextBox->setEnabled(enable);
    ui->passTextBox->setEnabled(enable);
    ui->buttonBox->setEnabled(enable);
}

// Enable the OK button only when both textboxes contain something.
void LoginDialog::on_userTextBox_textEdited(const QString &newText)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)
        ->setEnabled(!newText.isEmpty() && !ui->passTextBox->text().isEmpty());
}
void LoginDialog::on_passTextBox_textEdited(const QString &newText)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)
        ->setEnabled(!newText.isEmpty() && !ui->userTextBox->text().isEmpty());
}

void LoginDialog::onTaskFailed(const QString &reason)
{
    // Set message
    ui->label->setText("<span style='color:red'>" + reason + "</span>");

    // Re-enable user-interaction
    setUserInputsEnabled(true);
    ui->progressBar->setVisible(false);
}

void LoginDialog::onTaskSucceeded()
{
    QDialog::accept();
}

void LoginDialog::onTaskStatus(const QString &status)
{
    ui->label->setText(status);
}

void LoginDialog::onTaskProgress(qint64 current, qint64 total)
{
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(current);
}

// Public interface
AccountPtr LoginDialog::newAccount(QWidget *parent, QString msg)
{
    LoginDialog dlg(parent);
    dlg.ui->label->setText(msg);
    if (dlg.exec() == QDialog::Accepted)
    {
        return dlg.m_account;
    }
    return 0;
}
