#include "AppearancePage.h"

#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

#include "DesktopServices.h"
#include "FileSystem.h"
#include "settings/SettingsObject.h"
#include "ui/dialogs/ThemeEditorDialog.h"
#include "ui/themes/CustomTheme.h"
#include "ui/themes/IconThemeList.h"
#include "ui/themes/ThemeList.h"
#include "ui/themes/ThemeManager.h"
#include "ui/widgets/ConsolePreview.h"
#include "ui/widgets/IconThemeSelectionWidget.h"
#include "ui/widgets/ThemeSelectionWidget.h"

AppearancePage::AppearancePage(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("appearancePage"));
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_tabs = new QTabWidget(this);
    layout->addWidget(m_tabs);

    // theme zips can be dropped anywhere on the page
    setAcceptDrops(true);

    m_themeWidget = new ThemeSelectionWidget(m_tabs);
    m_themeTab = makeListTab(m_themeWidget, m_themeButtons, Kind::Colors);
    m_tabs->addTab(m_themeTab, QString());
    // at once, like in the setup wizard: the dialog has no Cancel to take it back
    connect(m_themeWidget, &ThemeSelectionWidget::choiceChanged, this, [](const ThemeManager::Choice &choice)
    {
        auto manager = APPLICATION->themeManager();
        manager->save(choice);
        manager->apply(choice);
    });
    connect(m_themeWidget, &ThemeSelectionWidget::choiceChanged, this, &AppearancePage::updateFileButtons);
    connect(m_themeWidget, &ThemeSelectionWidget::rowMenuRequested, this, [this](const QModelIndex &index, const QPoint &at) {
        showRowMenu(Kind::Colors, index, at);
    });

    m_iconsWidget = new IconThemeSelectionWidget(m_tabs);
    m_iconsTab = makeListTab(m_iconsWidget, m_iconsButtons, Kind::Icons);
    m_tabs->addTab(m_iconsTab, QString());
    connect(m_iconsWidget, &IconThemeSelectionWidget::iconThemeChanged, this, [this](const QString &id)
    {
        APPLICATION->settings()->set("IconTheme", id);
        APPLICATION->setIconTheme(id);
        updateFileButtons();
    });
    connect(m_iconsWidget, &IconThemeSelectionWidget::rowMenuRequested, this, [this](const QModelIndex &index, const QPoint &at) {
        showRowMenu(Kind::Icons, index, at);
    });
    // themes that come and go
    connect(model(Kind::Colors), &QAbstractItemModel::modelReset, this, &AppearancePage::updateFileButtons);
    connect(model(Kind::Icons), &QAbstractItemModel::modelReset, this, &AppearancePage::updateFileButtons);

    m_consoleTab = makeConsoleTab();
    m_tabs->addTab(m_consoleTab, QString());

    retranslate();
    loadSettings();
}

QWidget *AppearancePage::makeListTab(QWidget *list, FileButtons &buttons, Kind kind)
{
    // the list, and below it: Import and the folder of them all on the left, what can be
    // done with the chosen one on the right
    auto tab = new QWidget(m_tabs);
    auto layout = new QVBoxLayout(tab);
    layout->setContentsMargins(0, 0, 0, 0);
    list->setParent(tab);
    layout->addWidget(list, 1);
    auto row = new QHBoxLayout();
    // the list widgets have their own margins: line up with them
    row->setContentsMargins(list->layout()->contentsMargins().left(), 0, list->layout()->contentsMargins().right(),
                            list->layout()->contentsMargins().bottom());
    buttons.importButton = new QPushButton(tab);
    buttons.openFolder = new QPushButton(tab);
    buttons.exportButton = new QPushButton(tab);
    buttons.deleteButton = new QPushButton(tab);
    row->addWidget(buttons.importButton);
    row->addWidget(buttons.openFolder);
    row->addStretch(1);
    if(kind == Kind::Colors)
    {
        buttons.edit = new QPushButton(tab);
        row->addWidget(buttons.edit);
        connect(buttons.edit, &QPushButton::clicked, this, [this]() { editTheme(indexOf(Kind::Colors, currentId(Kind::Colors))); });
    }
    row->addWidget(buttons.exportButton);
    row->addWidget(buttons.deleteButton);
    layout->addLayout(row);

    connect(buttons.importButton, &QPushButton::clicked, this, &AppearancePage::importClicked);
    connect(buttons.openFolder, &QPushButton::clicked, this, [this, kind]() {
        DesktopServices::openDirectory(folder(kind));
    });
    connect(buttons.exportButton, &QPushButton::clicked, this, [this, kind]() {
        exportTheme(kind, indexOf(kind, currentId(kind)));
    });
    connect(buttons.deleteButton, &QPushButton::clicked, this, [this, kind]() {
        deleteTheme(kind, indexOf(kind, currentId(kind)));
    });
    return tab;
}

QAbstractItemModel *AppearancePage::model(Kind kind) const
{
    auto manager = APPLICATION->themeManager();
    if(kind == Kind::Colors)
        return manager->themeList();
    return manager->iconThemeList();
}

QString AppearancePage::folder(Kind kind) const
{
    auto manager = APPLICATION->themeManager();
    return kind == Kind::Colors ? manager->themeList()->folder() : manager->iconThemeList()->folder();
}

QModelIndex AppearancePage::indexOf(Kind kind, const QString &id) const
{
    auto manager = APPLICATION->themeManager();
    return kind == Kind::Colors ? manager->themeList()->indexOf(id) : manager->iconThemeList()->indexOf(id);
}

QString AppearancePage::currentId(Kind kind) const
{
    return kind == Kind::Colors ? m_themeWidget->choice().theme : m_iconsWidget->iconTheme();
}

bool AppearancePage::isUserTheme(const QModelIndex &index) const
{
    // the roles have the same numbers in both lists
    return index.isValid() && !index.data(ThemeList::BuiltInRole).toBool();
}

void AppearancePage::select(Kind kind, const QString &id)
{
    if(kind == Kind::Colors)
    {
        auto manager = APPLICATION->themeManager();
        auto choice = m_themeWidget->choice();
        choice.theme = id;
        m_themeWidget->setChoice(choice);
        manager->save(choice);
        manager->apply(choice);
        m_tabs->setCurrentWidget(m_themeTab);
    }
    else
    {
        m_iconsWidget->setIconTheme(id);
        APPLICATION->settings()->set("IconTheme", id);
        APPLICATION->setIconTheme(id);
        m_tabs->setCurrentWidget(m_iconsTab);
    }
    updateFileButtons();
}

void AppearancePage::updateFileButtons()
{
    for(auto kind : {Kind::Colors, Kind::Icons})
    {
        auto &buttons = kind == Kind::Colors ? m_themeButtons : m_iconsButtons;
        const bool user = isUserTheme(indexOf(kind, currentId(kind)));
        buttons.exportButton->setEnabled(user);
        buttons.deleteButton->setEnabled(user);
    }
    // any theme: a user theme is saved in place, any as a new one
    m_themeButtons.edit->setText(tr("Ed&it..."));
    m_themeButtons.edit->setToolTip(tr("Change the chosen theme's colors, or make a theme of your own from them"));
}

void AppearancePage::showRowMenu(Kind kind, const QModelIndex &index, const QPoint &globalPos)
{
    QMenu menu(this);
    // any theme that reads can be edited (a built-in one saved as a new one)
    const bool readable = index.isValid() && index.data(ThemeList::ErrorRole).toString().isEmpty();
    if(kind == Kind::Colors && readable)
    {
        menu.addAction(tr("Edit..."), this, [this, index]() { editTheme(index); });
        menu.addSeparator();
    }
    if(isUserTheme(index))
    {
        const QString id = index.data(ThemeList::IdRole).toString();
        menu.addAction(tr("Open folder"), this, [this, kind, id]() {
            DesktopServices::openDirectory(FS::PathCombine(folder(kind), id));
        });
        menu.addAction(tr("Export..."), this, [this, kind, index]() { exportTheme(kind, index); });
        menu.addSeparator();
        menu.addAction(tr("Delete"), this, [this, kind, index]() { deleteTheme(kind, index); });
    }
    else
    {
        // built-in ones have no folder of their own
        menu.addAction(kind == Kind::Colors ? tr("Open themes folder") : tr("Open icon themes folder"), this, [this, kind]() {
            DesktopServices::openDirectory(folder(kind));
        });
    }
    menu.exec(globalPos);
}

void AppearancePage::importClicked()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Import a theme"), QString(), tr("Theme zip files (*.zip)"));
    if(!path.isEmpty())
    {
        importZip(path);
    }
}

void AppearancePage::importZip(const QString &path)
{
    auto fail = [this, &path](const QString &reason) {
        QMessageBox::warning(this, tr("Theme not imported"), tr("%1 can't be imported as a theme.\n\n%2").arg(QFileInfo(path).fileName(), reason));
    };
    QString error;
    const auto contents = ThemeArchive::inspect(path, error);
    if(!contents)
    {
        fail(error);
        return;
    }
    const QModelIndex existing = indexOf(contents->kind, contents->id);
    if(existing.isValid() && !isUserTheme(existing))
    {
        fail(tr("It is named \"%1\", like a built-in theme. Rename the zip file or its folder.").arg(contents->id));
        return;
    }
    if(QFileInfo::exists(FS::PathCombine(folder(contents->kind), contents->id)))
    {
        // the name only where it says more than the folder's
        const QString name = existing.isValid() ? existing.data(Qt::DisplayRole).toString() : contents->id;
        const QString question = name == contents->id
            ? tr("There already is a theme in the folder \"%1\". Replace it with the imported one?").arg(contents->id)
            : tr("There already is a theme in the folder \"%1\": %2. Replace it with the imported one?").arg(contents->id, name);
        auto answer = QMessageBox::question(this, tr("Replace theme?"), question, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if(answer != QMessageBox::Yes)
        {
            return;
        }
    }
    if(!ThemeArchive::install(path, *contents, folder(contents->kind), error))
    {
        fail(error);
        return;
    }
    // in the list at once, and chosen, so it shows
    if(contents->kind == Kind::Colors)
        APPLICATION->themeManager()->themeList()->rescan();
    else
        APPLICATION->themeManager()->iconThemeList()->rescan();
    const QModelIndex imported = indexOf(contents->kind, contents->id);
    if(imported.isValid() && (imported.flags() & Qt::ItemIsEnabled))
    {
        select(contents->kind, contents->id);
    }
}

void AppearancePage::exportTheme(Kind kind, const QModelIndex &index)
{
    if(!isUserTheme(index))
    {
        return;
    }
    const QString id = index.data(ThemeList::IdRole).toString();
    const QString path = QFileDialog::getSaveFileName(this, tr("Export the theme"), id + ".zip", tr("Theme zip files (*.zip)"));
    if(path.isEmpty())
    {
        return;
    }
    QString error;
    if(!ThemeArchive::exportTo(FS::PathCombine(folder(kind), id), path, error))
    {
        QMessageBox::warning(this, tr("Theme not exported"), error);
    }
}

void AppearancePage::deleteTheme(Kind kind, const QModelIndex &index)
{
    if(!isUserTheme(index))
    {
        return;
    }
    const QString id = index.data(ThemeList::IdRole).toString();
    const QString name = index.data(Qt::DisplayRole).toString();
    const QString path = FS::PathCombine(folder(kind), id);
    auto answer = QMessageBox::question(this, tr("Delete theme?"),
                                        tr("Delete the theme \"%1\"? Its folder is deleted with everything in it:\n%2").arg(name, path),
                                        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if(answer != QMessageBox::Yes)
    {
        return;
    }
    const bool wasCurrent = id == currentId(kind);
    if(!FS::deletePath(path))
    {
        QMessageBox::warning(this, tr("Theme not deleted"), tr("The folder %1 couldn't be deleted completely.").arg(path));
    }
    if(kind == Kind::Colors)
        APPLICATION->themeManager()->themeList()->rescan();
    else
        APPLICATION->themeManager()->iconThemeList()->rescan();
    // the chosen one gone: back to the default, saved, so the setting doesn't point at nothing
    if(wasCurrent && !indexOf(kind, id).isValid())
    {
        select(kind, kind == Kind::Colors ? APPLICATION->themeManager()->defaultTheme() : QString("pe_colored"));
    }
}

void AppearancePage::editTheme(const QModelIndex &index)
{
    if(!index.isValid() || !index.data(ThemeList::ErrorRole).toString().isEmpty())
    {
        return;
    }
    auto manager = APPLICATION->themeManager();
    const QString id = index.data(ThemeList::IdRole).toString();
    const bool user = isUserTheme(index);

    // the theme as it looks with its own accent, every role
    ThemeManager::Choice own;
    own.theme = id;
    const QPalette palette = manager->previewPalette(id, own);
    ThemeWriter::Theme theme;
    const QString name = index.data(Qt::DisplayRole).toString();
    theme.name = name;
    for(auto &role : CustomTheme::colorRoles())
    {
        theme.colors[role.first] = palette.color(QPalette::Active, role.first);
    }
    if(auto source = manager->themeList()->theme(id))
    {
        theme.logColors = source->logColors();
    }

    // a copy can't take a built-in theme's id
    QStringList reserved;
    auto list = manager->themeList();
    for(int row = 0; row < list->rowCount(); row++)
    {
        const QModelIndex other = list->index(row);
        if(other.data(ThemeList::BuiltInRole).toBool())
            reserved.append(other.data(ThemeList::IdRole).toString());
    }
    const QString themeFolder = user ? FS::PathCombine(folder(Kind::Colors), id) : QString();
    // a user theme can be saved where it is; any, built-in ones too, as a new one
    ThemeEditorDialog dialog(folder(Kind::Colors), user ? id : QString(), themeFolder, theme, reserved, this);
    if(dialog.exec() != QDialog::Accepted)
    {
        return;
    }
    // in the list at once, and chosen, so it shows
    list->rescan();
    select(Kind::Colors, dialog.savedId());
}

void AppearancePage::dragEnterEvent(QDragEnterEvent *event)
{
    // one zip file at a time
    const auto urls = event->mimeData()->urls();
    if(urls.size() == 1 && urls.first().isLocalFile() && urls.first().toLocalFile().endsWith(".zip", Qt::CaseInsensitive))
    {
        event->acceptProposedAction();
    }
}

void AppearancePage::dropEvent(QDropEvent *event)
{
    const auto urls = event->mimeData()->urls();
    if(urls.size() != 1 || !urls.first().isLocalFile())
    {
        return;
    }
    event->acceptProposedAction();
    importZip(urls.first().toLocalFile());
}

QWidget *AppearancePage::makeConsoleTab()
{
    auto tab = new QWidget(m_tabs);
    auto layout = new QVBoxLayout(tab);
    auto fontRow = new QHBoxLayout();
    m_fontLabel = new QLabel(tab);
    m_consoleFont = new QFontComboBox(tab);
    m_consoleFont->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_fontSize = new QSpinBox(tab);
    m_fontSize->setRange(5, 16);
    m_fontLabel->setBuddy(m_consoleFont);
    fontRow->addWidget(m_fontLabel);
    fontRow->addWidget(m_consoleFont, 1);
    fontRow->addWidget(m_fontSize);
    layout->addLayout(fontRow);
    m_consolePreview = new ConsolePreview(tab);
    layout->addWidget(m_consolePreview, 1);

    connect(m_consoleFont, &QFontComboBox::currentFontChanged, this, &AppearancePage::consoleFontChanged);
    connect(m_fontSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &AppearancePage::consoleFontChanged);
    // the theme's log colours
    connect(APPLICATION->themeManager(), &ThemeManager::applied, this, &AppearancePage::updateConsolePreview);
    return tab;
}

void AppearancePage::consoleFontChanged()
{
    if(!m_loading)
    {
        auto s = APPLICATION->settings();
        s->set("ConsoleFont", m_consoleFont->currentFont().family());
        s->set("ConsoleFontSize", m_fontSize->value());
    }
    updateConsolePreview();
}

void AppearancePage::updateConsolePreview()
{
    m_consolePreview->setConsoleFont(m_consoleFont->currentFont().family(), m_fontSize->value());
    m_consolePreview->setLogColors(APPLICATION->themeManager()->logColors());
}

void AppearancePage::loadSettings()
{
    auto s = APPLICATION->settings();
    m_iconsWidget->setIconTheme(s->get("IconTheme").toString());
    m_themeWidget->setChoice(APPLICATION->themeManager()->savedChoice());

    m_loading = true;
    m_consoleFont->setCurrentFont(QFont(s->get("ConsoleFont").toString()));
    bool conversionOk = true;
    int fontSize = s->get("ConsoleFontSize").toInt(&conversionOk);
    m_fontSize->setValue(conversionOk ? fontSize : 11);
    m_loading = false;
    updateConsolePreview();
    updateFileButtons();
}

void AppearancePage::retranslate()
{
    m_tabs->setTabText(m_tabs->indexOf(m_themeTab), tr("Theme"));
    m_tabs->setTabText(m_tabs->indexOf(m_iconsTab), tr("Icons"));
    for(auto buttons : {&m_themeButtons, &m_iconsButtons})
    {
        buttons->importButton->setText(tr("&Import..."));
        buttons->importButton->setToolTip(tr("Add a theme or an icon theme from a zip file. Zip files can also be dropped here."));
        buttons->openFolder->setText(tr("Open &folder"));
        buttons->exportButton->setText(tr("&Export..."));
        buttons->exportButton->setToolTip(tr("Save the chosen theme as a zip file, to share it"));
        buttons->deleteButton->setText(tr("&Delete"));
    }
    m_themeButtons.openFolder->setToolTip(tr("Open the folder with the themes"));
    m_iconsButtons.openFolder->setToolTip(tr("Open the folder with the icon themes"));
    m_tabs->setTabText(m_tabs->indexOf(m_consoleTab), tr("Console"));
    m_fontLabel->setText(tr("&Font:"));
    updateConsolePreview();
}

void AppearancePage::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    QWidget::changeEvent(event);
}
