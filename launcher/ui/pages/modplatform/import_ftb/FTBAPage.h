/*
 * Copyright 2022 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QWidget>
#include <QTreeView>
#include <QTextBrowser>

#include "ui/pages/BasePage.h"
#include <Application.h>
#include "tasks/Task.h"
#include "QObjectPtr.h"

#include "Model.h"

class NewInstanceDialog;

namespace ImportFTB {

namespace Ui
{
class FTBAPage;
}

class Model;

class FTBAPage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit FTBAPage(NewInstanceDialog * dialog, QWidget *parent = 0);
    virtual ~FTBAPage();
    QString displayName() const override
    {
        return tr("FTBApp Import");
    }
    QIcon icon() const override
    {
        return APPLICATION->getThemedIcon("ftb_logo");
    }
    QString id() const override
    {
        return "import_ftb";
    }
    QString helpPage() const override
    {
        return "FTB-app";
    }
    bool shouldDisplay() const override;
    void openedImpl() override;

private:
    void suggestCurrent();
    void onPackSelectionChanged(Modpack *pack = nullptr);

private slots:
    void onPublicPackSelectionChanged(QModelIndex first, QModelIndex second);

private:
    QTreeView* currentList = nullptr;
    QTextBrowser* currentModpackInfo = nullptr;

    bool initialized = false;
    nonstd::optional<Modpack> selected;

    Model* packModel = nullptr;

    NewInstanceDialog* dialog = nullptr;

    Ui::FTBAPage *ui = nullptr;
};

}
