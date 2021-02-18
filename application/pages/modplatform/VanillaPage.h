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

#include "pages/BasePage.h"
#include <MultiMC.h>
#include "tasks/Task.h"

namespace Ui
{
class VanillaPage;
}

class NewInstanceDialog;

class VanillaPage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit VanillaPage(NewInstanceDialog *dialog, QWidget *parent = 0);
    virtual ~VanillaPage();
    virtual QString displayName() const override
    {
        return tr("Vanilla");
    }
    virtual QIcon icon() const override
    {
        return MMC->getThemedIcon("minecraft");
    }
    virtual QString id() const override
    {
        return "vanilla";
    }
    virtual QString helpPage() const override
    {
        return "Vanilla-platform";
    }
    virtual bool shouldDisplay() const override;
    void openedImpl() override;

    BaseVersionPtr selectedVersion() const;

public slots:
    void setSelectedVersion(BaseVersionPtr version);

private slots:
    void filterChanged();

private:
    void refresh();
    void suggestCurrent();

private:
    bool initialized = false;
    NewInstanceDialog *dialog = nullptr;
    Ui::VanillaPage *ui = nullptr;
    bool m_versionSetByUser = false;
    BaseVersionPtr m_selectedVersion;
};
