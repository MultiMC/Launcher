#include "GuiUtil.h"

#include <QClipboard>
#include <QApplication>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMap>
#include <QScreen>
#include <QStandardPaths>

#include "ui/dialogs/ProgressDialog.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "net/PasteUpload.h"

#include "Application.h"
#include <settings/SettingsObject.h>
#include <DesktopServices.h>
#include <BuildConfig.h>

QString GuiUtil::uploadPaste(const QString &text, QWidget *parentWidget)
{
    ProgressDialog dialog(parentWidget);
    auto APIKeySetting = APPLICATION->settings()->get("PasteEEAPIKey").toString();
    if(APIKeySetting == "multimc")
    {
        APIKeySetting = BuildConfig.PASTE_EE_KEY;
    }
    std::unique_ptr<PasteUpload> paste(new PasteUpload(parentWidget, text, APIKeySetting));

    if (!paste->validateText())
    {
        CustomMessageBox::selectable(
            parentWidget, QObject::tr("Upload failed"),
            QObject::tr("The log file is too big. You'll have to upload it manually."),
            QMessageBox::Warning)->exec();
        return QString();
    }

    dialog.execWithTask(paste.get());
    if (!paste->wasSuccessful())
    {
        CustomMessageBox::selectable(
            parentWidget,
            QObject::tr("Upload failed"),
            paste->failReason(),
            QMessageBox::Critical
        )->exec();
        return QString();
    }
    else
    {
        const QString link = paste->pasteLink();
        setClipboardText(link);
        CustomMessageBox::selectable(
            parentWidget, QObject::tr("Upload finished"),
            QObject::tr("The <a href=\"%1\">link to the uploaded log</a> has been placed in your clipboard.").arg(link),
            QMessageBox::Information)->exec();
        return link;
    }
}

void GuiUtil::setClipboardText(const QString &text)
{
    QApplication::clipboard()->setText(text);
}

namespace {
// setting -> where the dialog was, relative to its parent window, when it last closed in this run
QMap<QString, QPoint> &dialogOffsets()
{
    static QMap<QString, QPoint> offsets;
    return offsets;
}

// rect fits on one of the screens there are now
bool onAScreen(const QRect &rect)
{
    for(auto screen: QGuiApplication::screens())
    {
        if(screen->availableGeometry().contains(rect))
        {
            return true;
        }
    }
    return false;
}
}

void GuiUtil::restoreDialogGeometry(QWidget *dialog, const QString &setting)
{
    const auto geometry = QByteArray::fromBase64(APPLICATION->settings()->get(setting).toByteArray());
    if(!dialog->restoreGeometry(geometry))
    {
        return;
    }
    // the saved place is from an earlier run, maybe on a screen that is off or gone: forgetting the
    // move lets QDialog center it over its parent when it is shown, at the restored size
    dialog->setAttribute(Qt::WA_Moved, false);

    auto parent = dialog->parentWidget();
    if(!parent || !dialogOffsets().contains(setting))
    {
        return;
    }
    // moved again in this run: back where it was next to the parent window, following that
    // window to another screen, unless that would put it (partly) off the screens
    const QPoint place = parent->window()->pos() + dialogOffsets().value(setting);
    if(onAScreen(QRect(place, dialog->frameGeometry().size())))
    {
        dialog->move(place);
    }
}

void GuiUtil::saveDialogGeometry(QWidget *dialog, const QString &setting)
{
    APPLICATION->settings()->set(setting, dialog->saveGeometry().toBase64());
    if(auto parent = dialog->parentWidget())
    {
        dialogOffsets().insert(setting, dialog->pos() - parent->window()->pos());
    }
}

static QStringList BrowseForFileInternal(QString context, QString caption, QString filter, QString defaultPath, QWidget *parentWidget, bool single)
{
    static QMap<QString, QString> savedPaths;

    QFileDialog w(parentWidget, caption);
    QSet<QString> locations;
    auto f = [&](QStandardPaths::StandardLocation l)
    {
        QString location = QStandardPaths::writableLocation(l);
        QFileInfo finfo(location);
        if (!finfo.exists()) {
            return;
        }
        locations.insert(location);
    };
    f(QStandardPaths::DesktopLocation);
    f(QStandardPaths::DocumentsLocation);
    f(QStandardPaths::DownloadLocation);
    f(QStandardPaths::HomeLocation);
    QList<QUrl> urls;
    for (auto location : locations)
    {
        urls.append(QUrl::fromLocalFile(location));
    }
    urls.append(QUrl::fromLocalFile(defaultPath));

    w.setFileMode(single ? QFileDialog::ExistingFile : QFileDialog::ExistingFiles);
    w.setAcceptMode(QFileDialog::AcceptOpen);
    w.setNameFilter(filter);

    QString pathToOpen;
    if(savedPaths.contains(context))
    {
        pathToOpen = savedPaths[context];
    }
    else
    {
        pathToOpen = defaultPath;
    }
    if(!pathToOpen.isEmpty())
    {
        QFileInfo finfo(pathToOpen);
        if(finfo.exists() && finfo.isDir())
        {
            w.setDirectory(finfo.absoluteFilePath());
        }
    }

    w.setSidebarUrls(urls);

    if (w.exec())
    {
        savedPaths[context] = w.directory().absolutePath();
        return w.selectedFiles();
    }
    savedPaths[context] = w.directory().absolutePath();
    return {};
}

QString GuiUtil::BrowseForFile(QString context, QString caption, QString filter, QString defaultPath, QWidget *parentWidget)
{
    auto resultList = BrowseForFileInternal(context, caption, filter, defaultPath, parentWidget, true);
    if(resultList.size())
    {
        return resultList[0];
    }
    return QString();
}


QStringList GuiUtil::BrowseForFiles(QString context, QString caption, QString filter, QString defaultPath, QWidget *parentWidget)
{
    return BrowseForFileInternal(context, caption, filter, defaultPath, parentWidget, false);
}
