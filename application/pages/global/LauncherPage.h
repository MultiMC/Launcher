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

#include <memory>
#include <QDialog>

#include "java/JavaChecker.h"
#include "pages/BasePage.h"
#include <Launcher.h>
#include "ColorCache.h"
#include <translations/TranslationsModel.h>

#include <BuildConfig.h>

class QTextCharFormat;
class SettingsObject;

namespace Ui
{
class LauncherPage;
}

class LauncherPage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit LauncherPage(QWidget *parent = 0);
    ~LauncherPage();

    QString displayName() const override
    {
        return LAUNCHER_BUILD_NAME;
    }
    QIcon icon() const override
    {
        return LauncherPtr->getThemedIcon("launcher");
    }
    QString id() const override
    {
        return "launcher-settings";
    }
    QString helpPage() const override
    {
        return "launcher-settings";
    }
    bool apply() override;

private:
    void applySettings();
    void loadSettings();

private
slots:
    void on_instDirBrowseBtn_clicked();
    void on_modsDirBrowseBtn_clicked();
    void on_iconsDirBrowseBtn_clicked();

    /*!
     * Updates the list of update channels in the combo box.
     */
    void refreshUpdateChannelList();

    /*!
     * Updates the channel description label.
     */
    void refreshUpdateChannelDesc();

    /*!
     * Updates the font preview
     */
    void refreshFontPreview();

    void updateChannelSelectionChanged(int index);

private:
    Ui::LauncherPage *ui;

    /*!
     * Stores the currently selected update channel.
     */
    QString m_currentUpdateChannel;

    // default format for the font preview...
    QTextCharFormat *defaultFormat;

    std::unique_ptr<LogColorCache> m_colors;

    std::shared_ptr<TranslationsModel> m_languageModel;
};
