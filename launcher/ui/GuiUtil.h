#pragma once

#include <QWidget>

namespace GuiUtil
{
QString uploadPaste(const QString &text, QWidget *parentWidget);
void setClipboardText(const QString &text);
QStringList BrowseForFiles(QString context, QString caption, QString filter, QString defaultPath, QWidget *parentWidget);
QString BrowseForFile(QString context, QString caption, QString filter, QString defaultPath, QWidget *parentWidget);
/// a dialog's size and place, kept in setting (saveGeometry(), base64) when it closes. On the
/// first open after a start, the dialog gets its size back and opens over its parent: the screens
/// may have changed since. Later opens put it back where it was relative to the parent window, if
/// that place is still on a screen.
void restoreDialogGeometry(QWidget *dialog, const QString &setting);
void saveDialogGeometry(QWidget *dialog, const QString &setting);
}
