#include "ThemeEditorDialog.h"

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QShortcut>
#include <QStackedWidget>
#include <QTabWidget>
#include <QToolButton>
#include <QUndoStack>
#include <QVBoxLayout>

#include "AccentColorDialog.h"
#include "Application.h"
#include "DesktopServices.h"
#include "FileSystem.h"
#include "MessageLevel.h"
#include "ui/ColorCache.h"
#include "ui/themes/CustomTheme.h"
#include "ui/themes/ThemeManager.h"
#include "ui/widgets/ColorEdit.h"
#include "ui/widgets/ConsolePreview.h"
#include "ui/widgets/ThemePreview.h"
#include "ui/widgets/ThemeSampleWidget.h"

namespace {
QString tr(const char *text)
{
    return QCoreApplication::translate("ThemeEditorDialog", text);
}

// The roles in the order the editor shows them, with what they colour
QList<QPair<QPalette::ColorRole, QString>> roleLabels()
{
    return {
        {QPalette::Window, tr("Window background")},
        {QPalette::WindowText, tr("Window text")},
        {QPalette::Base, tr("Field and list background")},
        {QPalette::AlternateBase, tr("Alternate list rows")},
        {QPalette::Text, tr("Field and list text")},
        {QPalette::Button, tr("Button background")},
        {QPalette::ButtonText, tr("Button text")},
        {QPalette::Highlight, tr("Selection and accent")},
        {QPalette::HighlightedText, tr("Selected text")},
        {QPalette::Link, tr("Links")},
        {QPalette::ToolTipBase, tr("Tooltip background")},
        {QPalette::ToolTipText, tr("Tooltip text")},
        // not BrightText: nothing of ours uses it, nor Fusion (Qt only colours the title of a
        // floating dock window with it, in the classic Windows style). theme.json keeps it.
    };
}

// The log levels with what they colour
QList<QPair<QString, QString>> logLabels()
{
    return {
        {"Launcher", tr("Launcher messages")},
        {"Message", tr("Game messages")},
        {"Debug", tr("Debug messages")},
        {"Warning", tr("Warnings")},
        {"Error", tr("Errors")},
        {"Fatal", tr("Fatal errors")},
        {"FatalHighlight", tr("Fatal error background")},
    };
}

const qreal PREVIEW_SCALE = 1.5;

// tabs and what they show next to them, in the same order
enum Tab
{
    WidgetsTab,
    ConsoleTab
};
}

/**
 * A small window in a palette (ThemePreview), larger than the theme list's: for Quick setup
 */
class ThemePreviewWidget : public QWidget
{
public:
    using QWidget::QWidget;

    void setPreview(const QPalette &palette)
    {
        m_palette = palette;
        update();
    }

    QSize sizeHint() const override
    {
        return QSize(132 * PREVIEW_SCALE, 84 * PREVIEW_SCALE);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        // the window, centred, drawn at its usual size and scaled up
        const QSizeF size = QSizeF(sizeHint());
        p.translate((width() - size.width()) / 2, (height() - size.height()) / 2);
        p.scale(PREVIEW_SCALE, PREVIEW_SCALE);
        ThemePreview::paint(p, QRectF(0, 0, 132, 84), m_palette);
    }

private: /* data */
    QPalette m_palette;
};

/**
 * One change to the theme: the theme before and after it, put back with undo and redo. The
 * name's typing is one change until something else changes (mergeWith).
 */
class ThemeChange : public QUndoCommand
{
public:
    // ids for changes that merge: the name's typing
    static const int NAME = 1;

    ThemeChange(ThemeEditorDialog *editor, const ThemeWriter::Theme &before, const ThemeWriter::Theme &after, const QString &text,
                int id = -1)
        : QUndoCommand(text), m_editor(editor), m_before(before), m_after(after), m_id(id)
    {
    }

    void undo() override
    {
        m_editor->setTheme(m_before);
    }
    void redo() override
    {
        m_editor->setTheme(m_after);
    }
    int id() const override
    {
        return m_id;
    }
    bool mergeWith(const QUndoCommand *other) override
    {
        // the same id: the typing goes on
        m_after = static_cast<const ThemeChange *>(other)->m_after;
        return true;
    }

private: /* data */
    ThemeEditorDialog *m_editor;
    ThemeWriter::Theme m_before;
    ThemeWriter::Theme m_after;
    int m_id;
};

ThemeEditorDialog::ThemeEditorDialog(const QString &themesFolder, const QString &id, const QString &copyFrom,
                                     const ThemeWriter::Theme &initial, const QStringList &reserved, QWidget *parent)
    : QDialog(parent), m_folder(themesFolder), m_id(id), m_copyFrom(copyFrom), m_reserved(reserved), m_theme(initial)
{
    m_manager = APPLICATION->themeManager();
    m_original = m_theme;
    m_applied = m_theme;
    m_undoStack = new QUndoStack(this);
    setWindowTitle(m_id.isEmpty() ? tr("Edit theme (built in)") : tr("Edit theme"));
    auto layout = new QVBoxLayout(this);

    auto nameRow = new QHBoxLayout();
    auto nameLabel = new QLabel(tr("&Name:"), this);
    m_name = new QLineEdit(m_theme.name, this);
    nameLabel->setBuddy(m_name);
    nameRow->addWidget(nameLabel);
    nameRow->addWidget(m_name, 1);
    layout->addLayout(nameRow);
    // typing the name is a change like any other, merged until something else changes
    m_name->installEventFilter(this);
    connect(m_name, &QLineEdit::textEdited, this, [this](const QString &text) {
        m_theme.name = text.trimmed();
        changed(tr("Name"), ThemeChange::NAME);
    });

    // the colours on the left, what they colour on the right: each tab its own
    auto body = new QHBoxLayout();
    auto tabs = new QTabWidget(this);
    tabs->insertTab(WidgetsTab, makeWidgetsTab(), tr("&Widgets"));
    tabs->insertTab(ConsoleTab, makeConsoleTab(), tr("&Console"));
    body->addWidget(tabs);
    m_samples = new QStackedWidget(this);
    m_widgetSample = new ThemeSampleWidget(m_samples);
    m_samples->insertWidget(WidgetsTab, m_widgetSample);
    m_console = new ConsolePreview(m_samples);
    m_console->setConsoleFont(APPLICATION->settings()->get("ConsoleFont").toString(), APPLICATION->settings()->get("ConsoleFontSize").toInt());
    m_samples->insertWidget(ConsoleTab, m_console);
    connect(tabs, &QTabWidget::currentChanged, m_samples, &QStackedWidget::setCurrentIndex);
    body->addWidget(m_samples, 1);
    layout->addLayout(body, 1);

    // the button bar, in this order whatever the platform's dialogs do (a layout mirrors it for
    // right-to-left languages): the folder on its own at the start; at the end undo and redo,
    // the two saves together, and Close, which asks about changes not saved
    auto buttons = new QHBoxLayout();
    // the stylesheet and images are edited in the folder, where the theme already is
    if(!m_id.isEmpty())
    {
        auto openFolder = new QPushButton(tr("Open &folder"), this);
        openFolder->setToolTip(tr("Open the theme's folder, to edit its stylesheet and images"));
        connect(openFolder, &QPushButton::clicked, this, [this]() { DesktopServices::openDirectory(FS::PathCombine(m_folder, m_id)); });
        buttons->addWidget(openFolder);
    }
    buttons->addStretch(1);
    // undo and redo a step, or from their menus, back or forward to any
    m_undo = historyButton(tr("&Undo"), true);
    m_redo = historyButton(tr("&Redo"), false);
    buttons->addWidget(m_undo);
    buttons->addWidget(m_redo);
    connect(m_undoStack, &QUndoStack::indexChanged, this, &ThemeEditorDialog::updateHistoryButtons);
    m_save = new QPushButton(tr("&Save"), this);
    connect(m_save, &QPushButton::clicked, this, &ThemeEditorDialog::save);
    if(m_id.isEmpty())
    {
        m_save->setToolTip(tr("Built-in themes can't be changed: Save as... makes one of your own."));
    }
    // a theme of one's own, from any: built-in themes can only be saved this way
    auto saveAs = new QPushButton(tr("Save &as..."), this);
    saveAs->setToolTip(tr("Save as a new theme of your own"));
    connect(saveAs, &QPushButton::clicked, this, &ThemeEditorDialog::saveAs);
    auto close = new QPushButton(tr("&Close"), this);
    connect(close, &QPushButton::clicked, this, &ThemeEditorDialog::reject);
    buttons->addWidget(m_save);
    buttons->addWidget(saveAs);
    buttons->addWidget(close);
    // Enter doesn't save: it's easy to press while picking colours and typing the name
    for(auto button : {m_save, saveAs, close})
    {
        button->setAutoDefault(false);
    }
    layout->addLayout(buttons);

    // the usual keys too, in the text fields as well (eventFilter)
    connect(new QShortcut(QKeySequence::Undo, this), &QShortcut::activated, this, &ThemeEditorDialog::undo);
    connect(new QShortcut(QKeySequence::Redo, this), &QShortcut::activated, this, &ThemeEditorDialog::redo);
    // the platforms' redo keys differ (Ctrl+Shift+Z here, Ctrl+Y there): both everywhere
    for(auto keys : {QKeySequence(Qt::CTRL + Qt::Key_Y), QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z)})
    {
        if(!QKeySequence::keyBindings(QKeySequence::Redo).contains(keys))
            connect(new QShortcut(keys, this), &QShortcut::activated, this, &ThemeEditorDialog::redo);
    }

    updateControls();
    updateHistoryButtons();
    // the console's lines on one line each
    resize(qMax(sizeHint().width(), 860), sizeHint().height());
}

QWidget *ThemeEditorDialog::makeWidgetsTab()
{
    auto tab = new QWidget(this);
    auto layout = new QVBoxLayout(tab);
    auto quick = new QPushButton(tr("&Quick setup..."), tab);
    quick->setToolTip(tr("Set every color from a background, a text and an accent color"));
    connect(quick, &QPushButton::clicked, this, &ThemeEditorDialog::quickSetup);
    auto quickRow = new QHBoxLayout();
    quickRow->addWidget(quick);
    quickRow->addStretch(1);
    layout->addLayout(quickRow);

    auto grid = new QFormLayout();
    for(auto &role : roleLabels())
    {
        const QPalette::ColorRole which = role.first;
        auto edit = colorEdit(
            tab, role.second, [this, which]() { return paletteOf(m_theme.colors).color(QPalette::Active, which); },
            [this, which](const QColor &color) {
                ThemeWriter::Theme tried = m_theme;
                tried.colors[which] = color;
                return tried;
            },
            [this, which](const QColor &color) { m_theme.colors[which] = color; });
        m_roleEdits[which] = edit;
        grid->addRow(role.second + ":", edit);
    }
    layout->addLayout(grid);
    layout->addStretch(1);
    return tab;
}

QWidget *ThemeEditorDialog::makeConsoleTab()
{
    auto tab = new QWidget(this);
    auto layout = new QVBoxLayout(tab);
    auto grid = new QGridLayout();
    int row = 0;
    for(auto &level : logLabels())
    {
        const QString name = level.first;
        const QString title = level.second;
        auto edit = colorEdit(
            tab, title,
            [this, name]() {
                const QColor current = m_theme.logColors.value(name);
                return current.isValid() ? current : defaultLogColor(name);
            },
            [this, name](const QColor &color) {
                ThemeWriter::Theme tried = m_theme;
                tried.logColors[name] = color;
                return tried;
            },
            [this, name](const QColor &color) { m_theme.logColors[name] = color; });
        edit->setPlaceholderText(tr("Default"));
        auto reset = new QToolButton(tab);
        reset->setText(tr("Reset"));
        reset->setToolTip(tr("Use the console's own color"));
        m_logEdits[name] = edit;
        m_logResets[name] = reset;
        grid->addWidget(new QLabel(title + ":", tab), row, 0);
        grid->addWidget(edit, row, 1);
        grid->addWidget(reset, row, 2);
        row++;
        connect(reset, &QToolButton::clicked, this, [this, name, title]() {
            m_theme.logColors.remove(name);
            changed(tr("Reset %1").arg(title));
        });
    }
    grid->setColumnStretch(3, 1);
    layout->addLayout(grid);
    layout->addStretch(1);
    return tab;
}

ColorEdit *ThemeEditorDialog::colorEdit(QWidget *parent, const QString &title, std::function<QColor()> current,
                                        std::function<ThemeWriter::Theme(const QColor &)> tried, std::function<void(const QColor &)> set)
{
    auto edit = new ColorEdit(parent);
    // typed: shown as it comes, set when done; picked: set when the picker is closed with it
    connect(edit, &ColorEdit::colorTried, this, [this, tried](const QColor &color) { showSamples(tried(color)); });
    connect(edit, &ColorEdit::editingAbandoned, this, [this]() { showSamples(m_theme); });
    connect(edit, &ColorEdit::colorEdited, this, [this, set, title](const QColor &color) {
        set(color);
        changed(title);
    });
    connect(edit, &ColorEdit::pickRequested, this, [this, title, current, tried, set]() {
        const QColor picked = pick(this, current(), title, tried);
        if(picked.isValid())
        {
            set(picked);
            changed(title);
        }
    });
    edit->lineEdit()->installEventFilter(this);
    return edit;
}

bool ThemeEditorDialog::eventFilter(QObject *watched, QEvent *event)
{
    // The text fields have no undo of their own: one history for the whole editor
    auto field = qobject_cast<QLineEdit *>(watched);
    if(field && event->type() == QEvent::ShortcutOverride)
    {
        auto key = static_cast<QKeyEvent *>(event);
        const QKeySequence pressed(key->modifiers() | key->key());
        const bool undo = key->matches(QKeySequence::Undo);
        const bool redo = key->matches(QKeySequence::Redo) || pressed == QKeySequence(Qt::CTRL + Qt::Key_Y)
            || pressed == QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z);
        if(undo || redo)
        {
            // not overridden: the dialog's shortcut gets it
            event->ignore();
            return true;
        }
    }
    // their menus' Undo and Redo are the editor's too
    if(field && event->type() == QEvent::ContextMenu)
    {
        QMenu *menu = field->createStandardContextMenu();
        for(auto action : menu->actions())
        {
            const bool undo = action->objectName() == "edit-undo";
            if(undo || action->objectName() == "edit-redo")
            {
                auto ours = new QAction(action->text(), menu);
                ours->setShortcut(action->shortcut());
                ours->setEnabled(undo ? m_undoStack->canUndo() : m_undoStack->canRedo());
                connect(ours, &QAction::triggered, this, undo ? &ThemeEditorDialog::undo : &ThemeEditorDialog::redo);
                menu->insertAction(action, ours);
                menu->removeAction(action);
            }
        }
        menu->exec(static_cast<QContextMenuEvent *>(event)->globalPos());
        delete menu;
        return true;
    }
    return QDialog::eventFilter(watched, event);
}

QToolButton *ThemeEditorDialog::historyButton(const QString &text, bool undo)
{
    auto button = new QToolButton(this);
    button->setText(text);
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    button->setPopupMode(QToolButton::MenuButtonPopup);
    // as tall as the push buttons next to it
    button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    connect(button, &QToolButton::clicked, this, undo ? &ThemeEditorDialog::undo : &ThemeEditorDialog::redo);
    // the steps it can go through, the nearest first; one picked goes back (or forward) to it,
    // the ones in between too
    auto menu = new QMenu(button);
    connect(menu, &QMenu::aboutToShow, this, [this, menu, undo]() {
        menu->clear();
        settleTyping();
        const int index = m_undoStack->index();
        if(undo)
        {
            for(int step = index - 1; step >= 0; step--)
            {
                menu->addAction(m_undoStack->text(step), this, [this, step]() { m_undoStack->setIndex(step); });
            }
        }
        else
        {
            for(int step = index; step < m_undoStack->count(); step++)
            {
                menu->addAction(m_undoStack->text(step), this, [this, step]() { m_undoStack->setIndex(step + 1); });
            }
        }
    });
    button->setMenu(menu);
    return button;
}

void ThemeEditorDialog::updateHistoryButtons()
{
    m_undo->setEnabled(m_undoStack->canUndo());
    m_redo->setEnabled(m_undoStack->canRedo());
    m_undo->setToolTip(m_undoStack->canUndo() ? tr("Undo: %1").arg(m_undoStack->undoText()) : QString());
    m_redo->setToolTip(m_undoStack->canRedo() ? tr("Redo: %1").arg(m_undoStack->redoText()) : QString());
}

bool ThemeEditorDialog::settleTyping()
{
    // a colour typed and not set yet goes, rather than be undone around
    bool abandoned = false;
    for(auto edit : findChildren<ColorEdit *>())
    {
        abandoned |= edit->abandonEditing();
    }
    return abandoned;
}

void ThemeEditorDialog::placeAside(QWidget *dialog)
{
    // over the colours on the left, so the samples on the right stay in sight
    dialog->adjustSize();
    const QRect editor = frameGeometry();
    dialog->move(editor.left() + 24, editor.top() + qMax(0, (editor.height() - dialog->height()) / 2));
}

QColor ThemeEditorDialog::pick(QWidget *parent, const QColor &initial, const QString &title,
                               std::function<ThemeWriter::Theme(const QColor &)> tried)
{
    // the theme's colours to pick from, each once
    QList<QColor> suggestions;
    for(auto &color : m_theme.colors)
    {
        if(color.isValid() && !suggestions.contains(color))
            suggestions.append(color);
    }
    // the samples with the colour as it is picked; the picker's own preview, the window
    auto preview = [this, tried](const QColor &color) {
        const ThemeWriter::Theme theme = tried(color);
        showSamples(theme);
        return paletteOf(theme.colors);
    };
    AccentColorDialog dialog(initial, suggestions, preview, title, parent);
    if(parent == this)
    {
        placeAside(&dialog);
    }
    const bool accepted = dialog.exec() == QDialog::Accepted;
    // back to the theme as it is: a picked colour comes back with changed()
    showSamples(m_theme);
    return accepted ? dialog.color() : QColor();
}

void ThemeEditorDialog::quickSetup()
{
    const QPalette current = paletteOf(m_theme.colors);
    QColor background = current.color(QPalette::Active, QPalette::Window);
    QColor text = current.color(QPalette::Active, QPalette::WindowText);
    QColor accent = current.color(QPalette::Active, QPalette::Highlight);
    auto theme = [this](const QColor &background, const QColor &text, const QColor &accent) {
        ThemeWriter::Theme derived = m_theme;
        derived.colors = ThemeWriter::quickColors(background, text, accent);
        return derived;
    };

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Quick setup"));
    auto layout = new QVBoxLayout(&dialog);
    auto note = new QLabel(tr("Every color of the theme follows from these three. OK sets them all, as one change you can undo."), &dialog);
    note->setWordWrap(true);
    layout->addWidget(note);
    auto row = new QHBoxLayout();
    auto form = new QFormLayout();
    auto backgroundEdit = new ColorEdit(&dialog);
    auto textEdit = new ColorEdit(&dialog);
    auto accentEdit = new ColorEdit(&dialog);
    form->addRow(tr("Background:"), backgroundEdit);
    form->addRow(tr("Text:"), textEdit);
    form->addRow(tr("Accent:"), accentEdit);
    row->addLayout(form);
    auto preview = new ThemePreviewWidget(&dialog);
    row->addWidget(preview, 1);
    layout->addLayout(row);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    // the three, what follows from them in the dialog, and in the samples behind it
    // (typed colours show as they come, like picked ones)
    auto show = [&](const ThemeWriter::Theme &derived) {
        preview->setPreview(paletteOf(derived.colors));
        showSamples(derived);
    };
    auto update = [&]() {
        backgroundEdit->setColor(background);
        textEdit->setColor(text);
        accentEdit->setColor(accent);
        show(theme(background, text, accent));
    };
    auto connectEdit = [&](ColorEdit *edit, QColor &color, int which, const QString &title) {
        auto tried = [&, which](const QColor &one) {
            return theme(which == 0 ? one : background, which == 1 ? one : text, which == 2 ? one : accent);
        };
        connect(edit, &ColorEdit::colorTried, &dialog, [&, tried](const QColor &typed) { show(tried(typed)); });
        connect(edit, &ColorEdit::editingAbandoned, &dialog, [&]() { update(); });
        connect(edit, &ColorEdit::colorEdited, &dialog, [&](const QColor &typed) {
            color = typed;
            update();
        });
        connect(edit, &ColorEdit::pickRequested, &dialog, [&, title, tried]() {
            const QColor picked = pick(&dialog, color, title, tried);
            if(picked.isValid())
                color = picked;
            update();
        });
    };
    connectEdit(backgroundEdit, background, 0, tr("Background"));
    connectEdit(textEdit, text, 1, tr("Text"));
    connectEdit(accentEdit, accent, 2, tr("Accent"));
    update();
    placeAside(&dialog);

    if(dialog.exec() == QDialog::Accepted)
    {
        m_theme.colors = theme(background, text, accent).colors;
        changed(tr("Quick setup"));
        return;
    }
    showSamples(m_theme);
}

QColor ThemeEditorDialog::defaultLogColor(const QString &level) const
{
    const QPalette palette = paletteOf(m_theme.colors);
    LogColorCache colors(palette.color(QPalette::Text), palette.color(QPalette::Base));
    if(level == "FatalHighlight")
        return colors.getBack(MessageLevel::Fatal);
    const QMap<QString, MessageLevel::Enum> levels = {
        {"Launcher", MessageLevel::Launcher}, {"Message", MessageLevel::Message}, {"Debug", MessageLevel::Debug},
        {"Warning", MessageLevel::Warning},   {"Error", MessageLevel::Error},     {"Fatal", MessageLevel::Fatal},
    };
    return colors.getFront(levels.value(level, MessageLevel::Message));
}

QPalette ThemeEditorDialog::paletteOf(const QMap<QPalette::ColorRole, QColor> &colors) const
{
    return m_manager->finishedPalette(CustomTheme::paletteFrom(colors));
}

void ThemeEditorDialog::showSamples(const ThemeWriter::Theme &theme)
{
    const QPalette palette = paletteOf(theme.colors);
    m_widgetSample->setSamplePalette(palette);
    m_console->setPalette(palette);
    m_console->setLogColors(theme.logColors);
}

void ThemeEditorDialog::changed(const QString &text, int mergeId)
{
    // m_theme changed: from what was shown to it (pushing it runs redo, which shows it)
    m_undoStack->push(new ThemeChange(this, m_applied, m_theme, text, mergeId));
}

void ThemeEditorDialog::setTheme(const ThemeWriter::Theme &theme)
{
    m_theme = theme;
    m_applied = theme;
    // the field as it is while it's being typed in
    if(m_name->text().trimmed() != m_theme.name)
    {
        m_name->setText(m_theme.name);
    }
    updateControls();
}

void ThemeEditorDialog::undo()
{
    // a colour being typed is undone first: back to what it was
    if(settleTyping())
        return;
    m_undoStack->undo();
}

void ThemeEditorDialog::redo()
{
    settleTyping();
    m_undoStack->redo();
}

void ThemeEditorDialog::updateControls()
{
    // what the theme looks like: every role, even ones theme.json leaves out
    const QPalette palette = paletteOf(m_theme.colors);
    for(auto it = m_roleEdits.begin(); it != m_roleEdits.end(); ++it)
    {
        it.value()->setColor(palette.color(QPalette::Active, it.key()));
    }
    // the theme's log colours, or the console's own as they show on this theme
    for(auto it = m_logEdits.begin(); it != m_logEdits.end(); ++it)
    {
        const QColor color = m_theme.logColors.value(it.key());
        it.value()->setColor(color, defaultLogColor(it.key()));
        m_logResets[it.key()]->setEnabled(color.isValid());
    }
    showSamples(m_theme);
    m_save->setEnabled(!m_id.isEmpty() && !m_theme.name.isEmpty());
}

void ThemeEditorDialog::save()
{
    if(m_id.isEmpty())
    {
        return;
    }
    write(m_id, m_theme, QString());
}

void ThemeEditorDialog::saveAs()
{
    // the name as typed, or for an unchanged one, a copy's
    const QString original = m_original.name;
    const QString suggested = m_theme.name.isEmpty() || m_theme.name == original ? tr("%1 (copy)").arg(original) : m_theme.name;
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Save as"), tr("Name of the new theme:"), QLineEdit::Normal, suggested, &ok).trimmed();
    if(!ok || name.isEmpty())
    {
        return;
    }
    ThemeWriter::Theme theme = m_theme;
    theme.name = name;
    // a user theme's stylesheet and images go along
    write(ThemeWriter::freeId(m_folder, name, m_reserved), theme, m_copyFrom);
}

void ThemeEditorDialog::write(const QString &id, ThemeWriter::Theme theme, const QString &copyFrom)
{
    // every role written: what the theme looks like here is what it looks like everywhere
    const QPalette palette = CustomTheme::paletteFrom(theme.colors);
    for(auto &role : CustomTheme::colorRoles())
    {
        theme.colors[role.first] = palette.color(QPalette::Active, role.first);
    }
    QString error;
    if(!ThemeWriter::save(m_folder, id, theme, copyFrom, error))
    {
        QMessageBox::warning(this, tr("Theme not saved"), error);
        return;
    }
    m_savedId = id;
    m_undoStack->setClean();
    accept();
}

QString ThemeEditorDialog::savedId() const
{
    return m_savedId;
}

void ThemeEditorDialog::reject()
{
    // undone back to where it started: nothing to lose
    if(!m_undoStack->isClean())
    {
        auto answer = QMessageBox::question(this, tr("Discard changes?"), tr("The theme has changes that aren't saved. Discard them?"),
                                            QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel);
        if(answer != QMessageBox::Discard)
        {
            return;
        }
    }
    QDialog::reject();
}
