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

#include <QDialog>
#include "net/NetJob.h"

namespace Ui
{
class UpdateDialog;
}

enum UpdateAction
{
    UPDATE_LATER = QDialog::Rejected,
    UPDATE_NOW = QDialog::Accepted,
};

enum ChangelogType
{
    CHANGELOG_MARKDOWN,
    CHANGELOG_COMMITS
};

class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    /// `target` is what the changelog compares this build with: a branch or a commit
    explicit UpdateDialog(bool hasUpdate = true, QWidget *parent = 0, const QString &target = "develop");
    ~UpdateDialog();

    /// The update moves to another variant, and the user may object: a checkbox with `text`, ticked
    void offerVariantMove(const QString &text);
    /// The update moves to another variant, and the user can't stay: `text` says why
    void announceVariantMove(const QString &text);
    /// Whether the checkbox from offerVariantMove() was left ticked
    bool variantMoveAccepted() const;

public slots:
    void on_btnUpdateNow_clicked();
    void on_btnUpdateLater_clicked();

    /// Starts loading the changelog
    void loadChangelog();

    /// Slot for when the chengelog loads successfully.
    void changelogLoaded();

    /// Slot for when the chengelog fails to load...
    void changelogFailed(QString reason);

protected:
    void closeEvent(QCloseEvent * ) override;

private:
    Ui::UpdateDialog *ui;
    QByteArray changelogData;
    NetJob::Ptr dljob;
    ChangelogType m_changelogType = CHANGELOG_MARKDOWN;
    QString m_target;
};
