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

#include <QString>
#include <QIcon>
#include <memory>

#include "BasePageContainer.h"

class BasePage
{
public:
    virtual ~BasePage() {}
    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual QIcon icon() const = 0;
    virtual bool apply() { return true; }
    virtual bool shouldDisplay() const { return true; }
    virtual QString helpPage() const { return QString(); }
    void opened()
    {
        isOpened = true;
        openedImpl();
    }
    void closed()
    {
        isOpened = false;
        closedImpl();
    }
    virtual void openedImpl() {}
    virtual void closedImpl() {}
    virtual void setParentContainer(BasePageContainer * container)
    {
        m_container = container;
    };
public:
    int stackIndex = -1;
    int listIndex = -1;
protected:
    BasePageContainer * m_container = nullptr;
    bool isOpened = false;
};

typedef std::shared_ptr<BasePage> BasePagePtr;
