#include "ThemeEditorDialog.h"

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QShortcut>
#include <QStackedWidget>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include "AccentColorDialog.h"
#include "Application.h"
#include "DesktopServices.h"
#include "FileSystem.h"
#include "MessageLevel.h"
#include "ui/ColorCache.h"
#include "ui/themes/CustomTheme.h"
#include "ui/themes/ThemeManager.h"
#include "ui/widgets/ConsolePreview.h"
#include "ui/widgets/ThemePreview.h"
#include "ui/widgets/ThemeSampleWidget.h"

namespace {
QString tr(const char *text)
{
    return QCoreApplication::translate("ThemeEditorDialog", text);
}

// A colour button's picture: a rounded square of the colour, its edge a little darker, and a
// gap on the right so the button's text doesn't touch it; empty for an unset colour
QIcon swatch(const QColor &color)
{
    QPixmap pixmap(20, 16);
    pixmap.fill(Qt::transparent);
    if(!color.isValid())
    {
        return QIcon(pixmap);
    }
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color.darker(150), 1));
    p.setBrush(color);
    p.drawRoundedRect(QRectF(0.5, 0.5, 15, 15), 3, 3);
    return QIcon(pixmap);
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

ThemeEditorDialog::ThemeEditorDialog(const QString &themesFolder, const QString &id, const QString &copyFrom,
                                     const ThemeWriter::Theme &initial, const QStringList &reserved, QWidget *parent)
    : QDialog(parent), m_folder(themesFolder), m_id(id), m_copyFrom(copyFrom), m_reserved(reserved), m_theme(initial)
{
    m_manager = APPLICATION->themeManager();
    m_history.append(m_theme);
    setWindowTitle(m_id.isEmpty() ? tr("Edit theme (built in)") : tr("Edit theme"));
    auto layout = new QVBoxLayout(this);

    auto nameRow = new QHBoxLayout();
    auto nameLabel = new QLabel(tr("&Name:"), this);
    m_name = new QLineEdit(m_theme.name, this);
    nameLabel->setBuddy(m_name);
    nameRow->addWidget(nameLabel);
    nameRow->addWidget(m_name, 1);
    layout->addLayout(nameRow);
    // the name isn't an undo step: the field has its own
    connect(m_name, &QLineEdit::textEdited, this, [this](const QString &text) {
        m_theme.name = text.trimmed();
        m_modified = true;
        updateControls();
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
    m_undo = new QPushButton(tr("&Undo"), this);
    m_redo = new QPushButton(tr("&Redo"), this);
    connect(m_undo, &QPushButton::clicked, this, &ThemeEditorDialog::undo);
    connect(m_redo, &QPushButton::clicked, this, &ThemeEditorDialog::redo);
    buttons->addWidget(m_undo);
    buttons->addWidget(m_redo);
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
    for(auto button : {m_undo, m_redo, m_save, saveAs, close})
    {
        button->setAutoDefault(false);
    }
    layout->addLayout(buttons);

    // the usual keys too; in the name field, they undo its typing
    connect(new QShortcut(QKeySequence::Undo, this), &QShortcut::activated, this, &ThemeEditorDialog::undo);
    connect(new QShortcut(QKeySequence::Redo, this), &QShortcut::activated, this, &ThemeEditorDialog::redo);
    // the platforms' redo keys differ (Ctrl+Shift+Z here, Ctrl+Y there): both everywhere
    for(auto keys : {QKeySequence(Qt::CTRL + Qt::Key_Y), QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z)})
    {
        if(!QKeySequence::keyBindings(QKeySequence::Redo).contains(keys))
            connect(new QShortcut(keys, this), &QShortcut::activated, this, &ThemeEditorDialog::redo);
    }

    updateControls();
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
        auto button = colorButton(tab);
        m_roleButtons[role.first] = button;
        grid->addRow(role.second + ":", button);
        const QPalette::ColorRole which = role.first;
        const QString title = role.second;
        connect(button, &QToolButton::clicked, this, [this, which, title]() {
            const QColor picked = pick(this, paletteOf(m_theme.colors).color(QPalette::Active, which), title, [this, which](const QColor &color) {
                ThemeWriter::Theme tried = m_theme;
                tried.colors[which] = color;
                return tried;
            });
            if(picked.isValid())
            {
                m_theme.colors[which] = picked;
                changed();
            }
        });
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
        auto button = colorButton(tab);
        auto reset = new QToolButton(tab);
        reset->setText(tr("Reset"));
        reset->setToolTip(tr("Use the console's own color"));
        m_logButtons[name] = button;
        m_logResets[name] = reset;
        grid->addWidget(new QLabel(title + ":", tab), row, 0);
        grid->addWidget(button, row, 1);
        grid->addWidget(reset, row, 2);
        row++;
        connect(button, &QToolButton::clicked, this, [this, name, title]() {
            const QColor current = m_theme.logColors.value(name);
            const QColor picked = pick(this, current.isValid() ? current : defaultLogColor(name), title, [this, name](const QColor &color) {
                ThemeWriter::Theme tried = m_theme;
                tried.logColors[name] = color;
                return tried;
            });
            if(picked.isValid())
            {
                m_theme.logColors[name] = picked;
                changed();
            }
        });
        connect(reset, &QToolButton::clicked, this, [this, name]() {
            m_theme.logColors.remove(name);
            changed();
        });
    }
    grid->setColumnStretch(3, 1);
    layout->addLayout(grid);
    layout->addStretch(1);
    return tab;
}

QToolButton *ThemeEditorDialog::colorButton(QWidget *parent)
{
    auto button = new QToolButton(parent);
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    // wide enough for the colour's name, so the column doesn't jump
    button->setMinimumWidth(button->fontMetrics().width("#0000000") + 40);
    return button;
}

void ThemeEditorDialog::showColor(QToolButton *button, const QColor &color, const QString &unset)
{
    button->setIcon(swatch(color));
    button->setText(color.isValid() ? color.name() : unset);
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
    auto backgroundButton = colorButton(&dialog);
    auto textButton = colorButton(&dialog);
    auto accentButton = colorButton(&dialog);
    form->addRow(tr("Background:"), backgroundButton);
    form->addRow(tr("Text:"), textButton);
    form->addRow(tr("Accent:"), accentButton);
    row->addLayout(form);
    auto preview = new ThemePreviewWidget(&dialog);
    row->addWidget(preview, 1);
    layout->addLayout(row);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    // the three, what follows from them in the dialog, and in the samples behind it
    auto update = [&]() {
        showColor(backgroundButton, background);
        showColor(textButton, text);
        showColor(accentButton, accent);
        const ThemeWriter::Theme derived = theme(background, text, accent);
        preview->setPreview(paletteOf(derived.colors));
        showSamples(derived);
    };
    auto pickInto = [&](QToolButton *button, QColor &color, int which, const QString &title) {
        connect(button, &QToolButton::clicked, &dialog, [&, which, title]() {
            const QColor picked = pick(&dialog, color, title, [&, which](const QColor &tried) {
                return theme(which == 0 ? tried : background, which == 1 ? tried : text, which == 2 ? tried : accent);
            });
            if(picked.isValid())
                color = picked;
            update();
        });
    };
    pickInto(backgroundButton, background, 0, tr("Background"));
    pickInto(textButton, text, 1, tr("Text"));
    pickInto(accentButton, accent, 2, tr("Accent"));
    update();
    placeAside(&dialog);

    if(dialog.exec() == QDialog::Accepted)
    {
        m_theme.colors = theme(background, text, accent).colors;
        changed();
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

void ThemeEditorDialog::changed()
{
    // a new step after the one shown; the ones undone before it are gone
    m_history.resize(m_step + 1);
    m_history.append(m_theme);
    m_step++;
    m_modified = true;
    updateControls();
}

void ThemeEditorDialog::undo()
{
    if(m_step == 0)
        return;
    m_step--;
    // the name stays as typed
    const QString name = m_theme.name;
    m_theme = m_history[m_step];
    m_theme.name = name;
    m_modified = true;
    updateControls();
}

void ThemeEditorDialog::redo()
{
    if(m_step + 1 >= m_history.size())
        return;
    m_step++;
    const QString name = m_theme.name;
    m_theme = m_history[m_step];
    m_theme.name = name;
    m_modified = true;
    updateControls();
}

void ThemeEditorDialog::updateControls()
{
    // what the theme looks like: every role, even ones theme.json leaves out
    const QPalette palette = paletteOf(m_theme.colors);
    for(auto it = m_roleButtons.begin(); it != m_roleButtons.end(); ++it)
    {
        showColor(it.value(), palette.color(QPalette::Active, it.key()));
    }
    // the theme's log colours, or the console's own as they show on this theme
    for(auto it = m_logButtons.begin(); it != m_logButtons.end(); ++it)
    {
        const QColor color = m_theme.logColors.value(it.key());
        showColor(it.value(), color.isValid() ? color : defaultLogColor(it.key()));
        if(!color.isValid())
        {
            it.value()->setText(tr("Default"));
        }
        m_logResets[it.key()]->setEnabled(color.isValid());
    }
    showSamples(m_theme);
    m_undo->setEnabled(m_step > 0);
    m_redo->setEnabled(m_step + 1 < m_history.size());
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
    const QString original = m_history.first().name;
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
    m_modified = false;
    accept();
}

QString ThemeEditorDialog::savedId() const
{
    return m_savedId;
}

void ThemeEditorDialog::reject()
{
    if(m_modified)
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
