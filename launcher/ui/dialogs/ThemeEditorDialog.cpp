#include "ThemeEditorDialog.h"

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
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
        {QPalette::BrightText, tr("Bright text")},
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
}

/**
 * A small window in a palette (ThemePreview), larger than the theme list's
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
    setWindowTitle(m_id.isEmpty() ? tr("New theme") : tr("Edit theme"));
    auto layout = new QVBoxLayout(this);

    auto nameRow = new QHBoxLayout();
    auto nameLabel = new QLabel(tr("&Name:"), this);
    m_name = new QLineEdit(m_theme.name, this);
    nameLabel->setBuddy(m_name);
    nameRow->addWidget(nameLabel);
    nameRow->addWidget(m_name, 1);
    layout->addLayout(nameRow);
    connect(m_name, &QLineEdit::textEdited, this, [this](const QString &text) {
        m_theme.name = text.trimmed();
        changed();
    });

    // the colours on the left, how they look on the right
    auto body = new QHBoxLayout();
    auto tabs = new QTabWidget(this);
    tabs->addTab(makeQuickTab(), tr("&Quick"));
    tabs->addTab(makeColorsTab(), tr("&Every color"));
    tabs->addTab(makeConsoleTab(), tr("&Console"));
    body->addWidget(tabs);
    auto previews = new QVBoxLayout();
    m_preview = new ThemePreviewWidget(this);
    previews->addWidget(m_preview);
    m_console = new ConsolePreview(this);
    m_console->setConsoleFont(APPLICATION->settings()->get("ConsoleFont").toString(), APPLICATION->settings()->get("ConsoleFontSize").toInt());
    previews->addWidget(m_console, 1);
    body->addLayout(previews, 1);
    layout->addLayout(body, 1);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    // the stylesheet and images are edited in the folder, where the theme already is
    if(!m_id.isEmpty())
    {
        auto openFolder = m_buttons->addButton(tr("Open &folder"), QDialogButtonBox::ActionRole);
        openFolder->setToolTip(tr("Open the theme's folder, to edit its stylesheet and images"));
        connect(openFolder, &QPushButton::clicked, this, [this]() { DesktopServices::openDirectory(FS::PathCombine(m_folder, m_id)); });
    }
    connect(m_buttons, &QDialogButtonBox::accepted, this, &ThemeEditorDialog::save);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &ThemeEditorDialog::reject);
    layout->addWidget(m_buttons);

    updateControls();
    // the console's lines on one line each
    resize(qMax(sizeHint().width(), 760), sizeHint().height());
}

QWidget *ThemeEditorDialog::makeQuickTab()
{
    auto tab = new QWidget(this);
    auto layout = new QVBoxLayout(tab);
    auto note = new QLabel(tr("Every color follows from these three. Setting one sets all of them again; dial them in on the next tab."), tab);
    note->setWordWrap(true);
    layout->addWidget(note);
    auto form = new QFormLayout();
    m_background = colorButton(tab);
    m_text = colorButton(tab);
    m_accent = colorButton(tab);
    form->addRow(tr("Background:"), m_background);
    form->addRow(tr("Text:"), m_text);
    form->addRow(tr("Accent:"), m_accent);
    layout->addLayout(form);
    layout->addStretch(1);

    // one of the three picked: every role from them
    auto quick = [this](QToolButton *button, QPalette::ColorRole role, const QString &title) {
        connect(button, &QToolButton::clicked, this, [this, role, title]() {
            const QColor background = m_theme.colors.value(QPalette::Window);
            const QColor text = m_theme.colors.value(QPalette::WindowText);
            const QColor accent = m_theme.colors.value(QPalette::Highlight);
            auto derive = [role, background, text, accent](const QColor &color) {
                return ThemeWriter::quickColors(role == QPalette::Window ? color : background, role == QPalette::WindowText ? color : text,
                                                role == QPalette::Highlight ? color : accent);
            };
            const QColor picked = pick(m_theme.colors.value(role), title, [derive](QPalette &p, const QColor &color) {
                p = CustomTheme::paletteFrom(derive(color));
            });
            if(picked.isValid())
            {
                m_theme.colors = derive(picked);
                changed();
            }
        });
    };
    quick(m_background, QPalette::Window, tr("Background"));
    quick(m_text, QPalette::WindowText, tr("Text"));
    quick(m_accent, QPalette::Highlight, tr("Accent"));
    return tab;
}

QWidget *ThemeEditorDialog::makeColorsTab()
{
    auto tab = new QWidget(this);
    auto layout = new QVBoxLayout(tab);
    auto grid = new QFormLayout();
    for(auto &role : roleLabels())
    {
        auto button = colorButton(tab);
        m_roleButtons[role.first] = button;
        grid->addRow(role.second + ":", button);
        const QPalette::ColorRole which = role.first;
        const QString title = role.second;
        connect(button, &QToolButton::clicked, this, [this, which, title]() {
            const QColor picked = pick(m_theme.colors.value(which), title, [which](QPalette &p, const QColor &color) {
                p.setColor(which, color);
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
            const QColor picked = pick(current.isValid() ? current : defaultLogColor(name), title, nullptr);
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

QColor ThemeEditorDialog::pick(const QColor &initial, const QString &title, std::function<void(QPalette &, const QColor &)> apply)
{
    // the theme's colours to pick from, each once
    QList<QColor> suggestions;
    for(auto &color : m_theme.colors)
    {
        if(color.isValid() && !suggestions.contains(color))
            suggestions.append(color);
    }
    // what the window would look like with it; the log colours show in the console instead
    auto preview = [this, apply](const QColor &color) {
        QPalette p = CustomTheme::paletteFrom(m_theme.colors);
        if(apply)
            apply(p, color);
        return m_manager->finishedPalette(p);
    };
    AccentColorDialog dialog(initial, suggestions, preview, title, this);
    if(dialog.exec() != QDialog::Accepted)
    {
        return QColor();
    }
    return dialog.color();
}

QColor ThemeEditorDialog::defaultLogColor(const QString &level) const
{
    const QPalette palette = previewPalette();
    LogColorCache colors(palette.color(QPalette::Text), palette.color(QPalette::Base));
    if(level == "FatalHighlight")
        return colors.getBack(MessageLevel::Fatal);
    const QMap<QString, MessageLevel::Enum> levels = {
        {"Launcher", MessageLevel::Launcher}, {"Message", MessageLevel::Message}, {"Debug", MessageLevel::Debug},
        {"Warning", MessageLevel::Warning},   {"Error", MessageLevel::Error},     {"Fatal", MessageLevel::Fatal},
    };
    return colors.getFront(levels.value(level, MessageLevel::Message));
}

QPalette ThemeEditorDialog::previewPalette() const
{
    return m_manager->finishedPalette(CustomTheme::paletteFrom(m_theme.colors));
}

void ThemeEditorDialog::changed()
{
    m_modified = true;
    updateControls();
}

void ThemeEditorDialog::updateControls()
{
    // what the theme looks like: every role, even ones theme.json leaves out
    const QPalette palette = previewPalette();
    showColor(m_background, palette.color(QPalette::Window));
    showColor(m_text, palette.color(QPalette::WindowText));
    showColor(m_accent, palette.color(QPalette::Highlight));
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
    m_preview->setPreview(palette);
    m_console->setPalette(palette);
    m_console->setLogColors(m_theme.logColors);
    m_buttons->button(QDialogButtonBox::Save)->setEnabled(!m_theme.name.isEmpty());
}

void ThemeEditorDialog::save()
{
    // every role written: what the theme looks like here is what it looks like everywhere
    const QPalette palette = CustomTheme::paletteFrom(m_theme.colors);
    ThemeWriter::Theme theme = m_theme;
    for(auto &role : CustomTheme::colorRoles())
    {
        theme.colors[role.first] = palette.color(QPalette::Active, role.first);
    }
    const QString id = m_id.isEmpty() ? ThemeWriter::freeId(m_folder, theme.name, m_reserved) : m_id;
    QString error;
    if(!ThemeWriter::save(m_folder, id, theme, m_id.isEmpty() ? m_copyFrom : QString(), error))
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
