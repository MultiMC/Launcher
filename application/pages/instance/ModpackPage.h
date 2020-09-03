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

#pragma once

#include <QWidget>

#include "java/JavaChecker.h"
#include "BaseInstance.h"
#include <QObjectPtr.h>
#include "pages/BasePage.h"
#include "MultiMC.h"

namespace Ui
{
class ModpackPage;
}

class ModpackPage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit ModpackPage(BaseInstance *inst, QWidget *parent = 0);
    virtual ~ModpackPage();
    virtual QString displayName() const override
    {
        return tr("Modpack");
    }
    virtual QIcon icon() const override
    {
        return MMC->getThemedIcon("modpack");
    }
    virtual QString id() const override
    {
        return "modpack";
    }
    virtual bool apply() override;
    virtual QString helpPage() const override
    {
        return "Modpack-settings";
    }
    virtual bool shouldDisplay() const override;

private slots:
    void applySettings();
    void loadSettings();

    void activateUIClicked(bool checked);
    void updateState();

private:
    Ui::ModpackPage *ui;
    BaseInstance *m_instance;
    bool uiActivated = false;
};
