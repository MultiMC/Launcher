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

#include <QWidget>

#include "ui/pages/BasePage.h"
#include <Application.h>
#include "tasks/Task.h"
#include <modplatform/curseforge/CFPackIndex.h>

namespace Ui
{
class CFPage;
}

class NewInstanceDialog;

namespace CurseForge {
    class ListModel;
}

class CFPage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit CFPage(NewInstanceDialog* dialog, QWidget *parent = 0);
    virtual ~CFPage();
    virtual QString displayName() const override
    {
        return tr("CurseForge");
    }
    virtual QIcon icon() const override
    {
        return APPLICATION->getThemedIcon("curseforge");
    }
    virtual QString id() const override
    {
        return "curseforge";
    }
    virtual QString helpPage() const override
    {
        return "CurseForge-platform";
    }
    virtual bool shouldDisplay() const override;

    void openedImpl() override;

    bool eventFilter(QObject * watched, QEvent * event) override;

private:
    void suggestCurrent();
    void refreshRightPane();

private slots:
    void triggerSearch();
    void onSelectionChanged(QModelIndex first, QModelIndex second);
    void onVersionSelectionChanged(QString data);
    void forceDocumentLayout();

private:
    Ui::CFPage *ui = nullptr;
    NewInstanceDialog* dialog = nullptr;
    CurseForge::ListModel* listModel = nullptr;
    CurseForge::IndexedPack m_current;

    QString selectedVersion;
};
