/* Copyright 2013-2015 MultiMC Contributors
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

#include <QTabWidget>
#include <QTabBar>
#include <QHBoxLayout>

#include "widgets/AccountsWidget.h"

AccountListPage::AccountListPage(InstancePtr instance, QWidget *parent)
	: QWidget(parent)
{
	m_tabs = new QTabWidget(this);
	m_tabs->tabBar()->hide();
	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->addWidget(m_tabs);

	m_accountsWidget = new AccountsWidget("", instance, this);
	m_accountsWidget->setCancelEnabled(false);
	m_tabs->addTab(m_accountsWidget, QString());
}

AccountListPage::~AccountListPage()
{
}
