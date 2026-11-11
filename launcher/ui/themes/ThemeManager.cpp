#include "ThemeManager.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QGuiApplication>
#include <QPixmapCache>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOption>
#include <QToolBar>
#include <QToolButton>
#include <QVersionNumber>

#include "BrightTheme.h"
#include "FileSystem.h"
#include "FusionStyle.h"
#include "DarkTheme.h"
#include "SystemAppearance.h"
#include "SystemTheme.h"
#include "IconThemeList.h"
#include "ThemeList.h"
#include "TitleBar.h"

namespace {
// Windows' default blue
const QColor DEFAULT_ACCENT(0x00, 0x78, 0xd4);

QString styleName(ThemeManager::Style style)
{
    return style == ThemeManager::Style::Native ? "native" : "fusion";
}

QString accentSourceName(ThemeManager::AccentSource source)
{
    switch (source)
    {
    case ThemeManager::AccentSource::Theme:
        return "theme";
    case ThemeManager::AccentSource::System:
        return "system";
    case ThemeManager::AccentSource::Custom:
        return "custom";
    }
    return "theme";
}

// The accent as the desktops use it: selections filled with the accent itself, their text
// black or white, whichever reads better on it; links in the accent's hue, made readable on
// base (text on a background, so they need the contrast)
void applyAccent(QPalette &p, const Colors::Accent &accent)
{
    if (!accent.isValid())
        return;
    const QColor base = p.color(QPalette::Base);
    p.setColor(QPalette::Highlight, accent.color);
    p.setColor(QPalette::HighlightedText, Colors::isDark(accent.color) ? Qt::white : Qt::black);
    p.setColor(QPalette::Link, Colors::readable(accent.color, base));
    p.setColor(QPalette::LinkVisited, Colors::readable(Colors::isDark(base) ? accent.light2 : accent.dark2, base));
}

// The disabled colours of a palette, from its active ones by one rule (setColor without a
// group sets the disabled colour too, the same as the active one):
// - foregrounds and accent fills fade to their background: its hue and saturation, their own
//   lightness partway to the background's (halfway for text, most of the way for fills), so a
//   disabled indicator or progress bar has no accent left, and no in-between hues appear;
// - fields and buttons most of the way to the window colour: the theme's own colours, flat.
// Window and the shading roles stay: Fusion draws outlines from Window.
void deriveDisabled(QPalette &p)
{
    auto active = [&p](QPalette::ColorRole role) { return p.color(QPalette::Active, role); };
    auto fadeTo = [&](QPalette::ColorRole role, QPalette::ColorRole background, qreal amount) {
        p.setColor(QPalette::Disabled, role, Colors::disabled(active(role), active(background), amount));
    };
    auto flatten = [&](QPalette::ColorRole role, qreal amount) {
        p.setColor(QPalette::Disabled, role, Colors::mix(active(role), active(QPalette::Window), amount));
    };
    fadeTo(QPalette::WindowText, QPalette::Window, 0.5);
    fadeTo(QPalette::Text, QPalette::Base, 0.5);
    fadeTo(QPalette::ButtonText, QPalette::Button, 0.5);
    fadeTo(QPalette::ToolTipText, QPalette::ToolTipBase, 0.5);
    fadeTo(QPalette::BrightText, QPalette::Window, 0.5);
    fadeTo(QPalette::Link, QPalette::Base, 0.5);
    fadeTo(QPalette::LinkVisited, QPalette::Base, 0.5);
    fadeTo(QPalette::Highlight, QPalette::Window, 0.6);
    // after Highlight's: on the disabled selection
    p.setColor(QPalette::Disabled, QPalette::HighlightedText,
               Colors::disabled(active(QPalette::HighlightedText), p.color(QPalette::Disabled, QPalette::Highlight), 0.5));
    flatten(QPalette::Base, 0.7);
    flatten(QPalette::AlternateBase, 0.7);
    flatten(QPalette::Button, 0.6);
}

// What the platform does to its own colour in another group (inactive window, disabled
// widget), done to ours: the same change in HSL saturation and lightness, keeping our hue.
// Desktops have their own effects for this (KDE's colour schemes change intensity and
// contrast, not just mix), and this follows them without knowing them.
QColor followPlatform(const QColor &ours, const QColor &platformActive, const QColor &platformOther)
{
    const QColor a = platformActive.toHsl(), o = platformOther.toHsl(), c = ours.toHsl();
    auto scaled = [](qreal value, qreal from, qreal to) {
        // no ratio from nothing: take the platform's value as it is
        return from > 0.001 ? qBound<qreal>(0.0, value * to / from, 1.0) : to;
    };
    const qreal saturation = scaled(c.hslSaturationF(), a.hslSaturationF(), o.hslSaturationF());
    const qreal lightness = scaled(c.lightnessF(), a.lightnessF(), o.lightnessF());
    return QColor::fromHslF(qMax<qreal>(0.0, c.hslHueF()), saturation, lightness).toRgb();
}

// The widget classes the platform theme can give palettes of their own (Qt 5.15,
// QApplicationPrivate::initializeWidgetPalettesFromTheme)
const char *const THEMED_CLASSES[] = {"QToolButton", "QAbstractButton", "QCheckBox", "QRadioButton", "QHeaderView",
                                      "QAbstractItemView", "QMessageBoxLabel", "QTabBar", "QLabel", "QGroupBox",
                                      "QMenu", "QMenuBar", "QTextEdit", "QTextControl", "QLineEdit"};

// The toolbars' overflow buttons (QToolBarExtension) ask the style for their arrow only when
// they're made, so they kept the arrow of the style the Launcher started with: give them the
// current style's (GH-5665)
void refreshToolBarExtensions()
{
    for (QWidget *widget : QApplication::allWidgets())
    {
        auto toolBar = qobject_cast<QToolBar *>(widget);
        if (!toolBar)
            continue;
        auto button = toolBar->findChild<QToolButton *>("qt_toolbar_ext_button", Qt::FindDirectChildrenOnly);
        if (!button)
            continue;
        QStyleOption option;
        option.initFrom(button);
        const auto arrow = toolBar->orientation() == Qt::Horizontal ? QStyle::SP_ToolBarHorizontalExtensionButton
                                                                     : QStyle::SP_ToolBarVerticalExtensionButton;
        button->setIcon(button->style()->standardIcon(arrow, &option));
    }
}

// A custom accent on the platform's palette (Native, System theme): our colours where the
// platform has its accent, in every group the way the platform makes its own (on KDE an
// inactive window's selection goes dim and greyish, a disabled one almost disappears). Only
// these roles are set, so everything else keeps following the platform. Put on p: by default
// nothing (a palette with no roles set), or a class palette (see applyNative).
QPalette accentOnPlatform(const QPalette &platform, const Colors::Accent &accent, QPalette p = QPalette())
{
    QPalette withBase = p;
    withBase.setColor(QPalette::Base, platform.color(QPalette::Base));
    applyAccent(withBase, accent);
    for (auto role : {QPalette::Highlight, QPalette::Link, QPalette::LinkVisited})
    {
        const QColor active = withBase.color(QPalette::Active, role);
        p.setColor(QPalette::Active, role, active);
        // inactive: dimmed like the desktop dims its own, our hue kept
        p.setColor(QPalette::Inactive, role,
                   followPlatform(active, platform.color(QPalette::Active, role), platform.color(QPalette::Inactive, role)));
        // disabled: faded to the window's hue (see deriveDisabled), as light as the desktop's
        // own disabled colour for this role
        p.setColor(QPalette::Disabled, role,
                   Colors::inHueOf(platform.color(QPalette::Disabled, QPalette::Window),
                                   platform.color(QPalette::Disabled, role).toHsl().lightnessF()));
    }
    // selected text: black or white, whichever reads better on each group's selection; a grey
    // when disabled
    for (auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled})
    {
        const QColor highlight = p.color(group, QPalette::Highlight);
        QColor text = Colors::isDark(highlight) ? Qt::white : Qt::black;
        if (group == QPalette::Disabled)
            text = Colors::disabled(text, highlight, 0.5);
        p.setColor(group, QPalette::HighlightedText, text);
    }
    return p;
}

// Breeze draws tool bars ("tools area") in the KDE colour scheme's Header colours, from the
// scheme file, not the palette: our palettes left them in the desktop's colours (white tool
// bar text on Bright). Like KDE applications with a colour scheme of their own
// (KColorSchemeManager), we write ours as a scheme file and point the application's
// KDE_COLOR_SCHEME_PATH at it; Breeze reloads it when the application palette changes.
const char *KDE_SCHEME_PROPERTY = "KDE_COLOR_SCHEME_PATH";

QByteArray kdeColorScheme(const QPalette &p)
{
    auto rgb = [&p](QPalette::ColorRole role) {
        const QColor c = p.color(QPalette::Active, role);
        return QString("%1,%2,%3").arg(c.red()).arg(c.green()).arg(c.blue());
    };
    QString out = "[General]\nColorScheme=MultiMC\nName=MultiMC\n";
    auto set = [&](const char *name, QPalette::ColorRole background, QPalette::ColorRole alternate,
                   QPalette::ColorRole foreground) {
        out += QString("\n[Colors:%1]\n").arg(name);
        out += "BackgroundNormal=" + rgb(background) + "\n";
        out += "BackgroundAlternate=" + rgb(alternate) + "\n";
        out += "ForegroundNormal=" + rgb(foreground) + "\n";
        out += "ForegroundInactive=" + rgb(foreground) + "\n";
        out += "ForegroundActive=" + rgb(QPalette::Highlight) + "\n";
        out += "ForegroundLink=" + rgb(QPalette::Link) + "\n";
        out += "ForegroundVisited=" + rgb(QPalette::LinkVisited) + "\n";
        out += "ForegroundNegative=" + rgb(QPalette::BrightText) + "\n";
        out += "ForegroundNeutral=" + rgb(foreground) + "\n";
        out += "ForegroundPositive=" + rgb(foreground) + "\n";
        out += "DecorationFocus=" + rgb(QPalette::Highlight) + "\n";
        out += "DecorationHover=" + rgb(QPalette::Highlight) + "\n";
    };
    set("View", QPalette::Base, QPalette::AlternateBase, QPalette::Text);
    set("Window", QPalette::Window, QPalette::Window, QPalette::WindowText);
    set("Header", QPalette::Window, QPalette::Window, QPalette::WindowText);
    set("Button", QPalette::Button, QPalette::Button, QPalette::ButtonText);
    set("Selection", QPalette::Highlight, QPalette::Highlight, QPalette::HighlightedText);
    set("Tooltip", QPalette::ToolTipBase, QPalette::ToolTipBase, QPalette::ToolTipText);
    set("Complementary", QPalette::Window, QPalette::Window, QPalette::WindowText);
    return out.toUtf8();
}
}

bool ThemeManager::Choice::operator==(const Choice &other) const
{
    return theme == other.theme && style == other.style && accentSource == other.accentSource
        && accentColor == other.accentColor;
}

ThemeManager::ThemeManager(SettingsObjectPtr settings, QObject *parent) : QObject(parent), m_settings(settings)
{
    m_nativeStyle = QApplication::style()->objectName();
    m_platformPalette = QApplication::palette();
    for (const char *name : THEMED_CLASSES)
    {
        m_platformClassPalettes.append(qMakePair(name, QApplication::palette(name)));
    }
    // the Qt we run with, not the one we were built with: Linux builds use the system's
    m_paletteResetWorks = QVersionNumber::fromString(qVersion()) >= QVersionNumber(5, 15);

    m_system = new SystemAppearance(this);
    m_builtIn.emplace_back(new SystemTheme(m_system));
    m_builtIn.emplace_back(new BrightTheme());
    m_builtIn.emplace_back(new DarkTheme());
    QList<ITheme *> listed;
    for (auto &theme : m_builtIn)
    {
        // System where there's something to follow
        if (theme->id() != "system" || canFollowSystem())
            listed.append(theme.get());
    }
    m_themeList = new ThemeList(listed, "themes", this);
    m_iconThemeList = new IconThemeList("iconthemes", this);
    // the current theme changed on disk: show it at once (theme authors edit and watch)
    connect(m_themeList, &ThemeList::themeReloaded, this, [this](const QString &id) {
        if (!m_current.theme.isEmpty() && m_current.theme == id)
        {
            apply(m_current);
        }
    });
    // or it went away, or broke: whatever applies instead
    connect(m_themeList, &QAbstractItemModel::modelReset, this, [this]() {
        if (!m_current.theme.isEmpty() && !theme(m_current.theme))
        {
            apply(m_current);
        }
    });

    connect(m_system, &SystemAppearance::changed, this, [this]() {
        // nothing applied yet: the start applies it
        const Choice now = effective(m_current);
        if (!m_current.theme.isEmpty() && (now.theme == "system" || now.accentSource == AccentSource::System))
        {
            apply(m_current);
        }
        emit systemChanged();
    });
    qDebug() << "Native style:" << m_nativeStyle << "- following the system:" << canFollowSystem();
}

ThemeManager::~ThemeManager()
{
}

QString ThemeManager::defaultTheme() const
{
    return canFollowSystem() ? "system" : "bright";
}

ThemeManager::Choice ThemeManager::savedChoice() const
{
    Choice choice;
    choice.theme = m_settings->get("Theme").toString();
    if (choice.theme.isEmpty())
    {
        choice.theme = defaultTheme();
    }
    const QString style = m_settings->get("ThemeStyle").toString();
    if (style == "native")
        choice.style = Style::Native;
    else if (style == "fusion")
        choice.style = Style::Fusion;
    else
        choice.style = nativeByDefault() ? Style::Native : Style::Fusion;
    const QString source = m_settings->get("AccentSource").toString();
    if (source == "theme")
        choice.accentSource = AccentSource::Theme;
    else if (source == "system")
        choice.accentSource = AccentSource::System;
    else if (source == "custom")
        choice.accentSource = AccentSource::Custom;
    else
        choice.accentSource = systemAccent().isValid() ? AccentSource::System : AccentSource::Theme;
    choice.accentColor = QColor(m_settings->get("AccentColor").toString());
    if (!choice.accentColor.isValid())
    {
        choice.accentColor = DEFAULT_ACCENT;
    }
    return choice;
}

void ThemeManager::save(const Choice &choice)
{
    m_settings->set("Theme", choice.theme);
    m_settings->set("ThemeStyle", styleName(choice.style));
    m_settings->set("AccentSource", accentSourceName(choice.accentSource));
    m_settings->set("AccentColor", choice.accentColor.name());
}

bool ThemeManager::themeChosen() const
{
    return !m_settings->get("Theme").toString().isEmpty();
}

ThemeManager::Choice ThemeManager::current() const
{
    return m_current;
}

ThemeList *ThemeManager::themeList() const
{
    return m_themeList;
}

IconThemeList *ThemeManager::iconThemeList() const
{
    return m_iconThemeList;
}

QMap<QString, QColor> ThemeManager::logColors() const
{
    ITheme *current = theme(effective(m_current).theme);
    return current ? current->logColors() : QMap<QString, QColor>();
}

ITheme *ThemeManager::theme(const QString &id) const
{
    // System also where it isn't listed: effective() replaces it there
    for (auto &builtIn : m_builtIn)
    {
        if (builtIn->id() == id)
            return builtIn.get();
    }
    return m_themeList->theme(id);
}

ThemeManager::Choice ThemeManager::effective(Choice choice) const
{
    // System where there's nothing to follow (Linux without the portal), or a theme that's gone
    if ((choice.theme == "system" && !canFollowSystem()) || !theme(choice.theme))
    {
        choice.theme = "bright";
    }
    if (choice.style == Style::Native && !hasNativeStyleChoice())
    {
        choice.style = Style::Fusion;
    }
    return choice;
}

Colors::Accent ThemeManager::accentFor(const Choice &choice) const
{
    // a native style that does its own: nothing from us
    if (!accentChoiceAvailable(choice.style))
        return Colors::Accent();
    switch (choice.accentSource)
    {
    case AccentSource::Theme:
        return theme(choice.theme)->accent();
    case AccentSource::System:
        return systemAccent();
    case AccentSource::Custom:
        // kept as the choice, but where it can't apply, the system's
        if (!customAccentAvailable(choice.style))
            return systemAccent();
        return Colors::Accent::fromColor(choice.accentColor);
    }
    return Colors::Accent();
}

// Our palettes, finished: the accent on top, then the disabled colours from the result
QPalette ThemeManager::finished(ITheme *theme, const Choice &choice) const
{
    QPalette p = theme->palette();
    applyAccent(p, accentFor(choice));
    deriveDisabled(p);
    return p;
}

QPalette ThemeManager::finishedPalette(const QPalette &themePalette) const
{
    QPalette p = themePalette;
    deriveDisabled(p);
    return p;
}

QPalette ThemeManager::previewPalette(const QString &themeId, const Choice &choice) const
{
    Choice preview = choice;
    preview.theme = themeId;
    preview = effective(preview);
    // the platform's look: the colours it started with, and a custom accent on them where the
    // style takes one (see applyNative)
    if (preview.style == Style::Native && (preview.theme == "system" || !nativeAbilities().themes))
    {
        if (preview.accentSource == AccentSource::Custom && customAccentAvailable(preview.style))
        {
            return accentOnPlatform(m_platformPalette, accentFor(preview), m_platformPalette);
        }
        return m_platformPalette;
    }
    preview.style = Style::Fusion;
    return finished(theme(preview.theme), preview);
}

void ThemeManager::apply(const Choice &choice)
{
    m_current = choice;
    const Choice now = effective(choice);
    ITheme *chosen = theme(now.theme);
    // Fusion (Qt 5.6) caches painted buttons, combo boxes, spin boxes... in QPixmapCache by
    // state and size, not palette: without this, they keep the old theme's colours. Newer Qt
    // has the palette in the key; drop this after checking the version we upgrade to.
    QPixmapCache::clear();
    // the old theme's stylesheet goes before the style changes: replacing the style under an
    // application stylesheet crashed in QApplication::setStyle (a theme with a stylesheet,
    // then any other theme, with Breeze)
    if (!qApp->styleSheet().isEmpty())
    {
        qApp->setStyleSheet(QString());
        forgetStyleSheetFonts();
    }
    if (now.style == Style::Native)
    {
        applyNative(chosen, now);
    }
    else
    {
        applyFusion(chosen, now);
    }
    // the stylesheet's theme: paths are this theme's resources, before the stylesheet loads them:
    // the other way round, themes using the same file names (a theme family) got the last
    // theme's images
    QDir::setSearchPaths("theme", chosen->searchPaths());
    qApp->setStyleSheet(chosen->styleSheet());
    refreshToolBarExtensions();
    // also when the palette stays the same: Dark -> System in a dark Windows gives the same
    // palette, but the title bars go from the palette's colours to the system's
    TitleBarWatcher::setStyle(now.style == Style::Native ? TitleBar::Style::Default : chosen->titleBarStyle());
    emit applied();
}

// Workaround for a Qt 5 bug (seen with 5.15; QTBUG-58221 is one case of it): a stylesheet's
// fonts (Renoise: "QWidget { font: bold 8pt ... }") are written into the widgets' own fonts,
// and Qt gives back the old ones only for widgets it styles again when the stylesheet goes:
// the rest (lists, viewports, spin box editors, widgets on hidden pages) kept them, in the next
// theme too. Fonts set in the code mark the widget (WA_SetFont) and stay; every other widget
// goes back to the font it inherits. The next stylesheet sets its own fonts after this.
// Check again with Qt 6, and drop this if it is fixed there.
void ThemeManager::forgetStyleSheetFonts()
{
    for (auto widget : QApplication::allWidgets())
    {
        if (!widget->testAttribute(Qt::WA_SetFont))
        {
            widget->setFont(QFont());
        }
    }
}

void ThemeManager::applyFusion(ITheme *theme, const Choice &choice)
{
    QApplication::setStyle(new FusionStyle(m_nativeStyle));
    qApp->setProperty(KDE_SCHEME_PROPERTY, QVariant());
    // before the System theme reads the system's colours: in the system's appearance
    TitleBar::setApplicationAppearance(TitleBar::Appearance::System);
    setOurPalette(finished(theme, choice));
}

// Ours for every widget class too: the platform theme can give classes palettes of their own
// (KDE's for tool buttons, menus, item views... in the desktop's colour scheme), which an
// application palette doesn't replace. A Bright theme on a dark KDE had white tool bar text.
void ThemeManager::setOurPalette(const QPalette &palette)
{
    QApplication::setPalette(palette);
    for (const char *name : THEMED_CLASSES)
    {
        QApplication::setPalette(palette, name);
    }
}

// The platform's style (macintosh, Breeze, Kvantum, or whatever else the desktop gives).
// - System: the platform theme's palette, all of it, which follows the desktop live (macOS's
//   appearance, KDE's colour scheme and accent). An empty palette (no roles set) gives it
//   back: Qt 5.15 resolves the application palette against the platform theme's.
// - Bright, Dark, Custom: macOS forces the app's appearance and keeps the platform palette for
//   Bright and Dark; elsewhere (Linux), our palettes on the native style, which Breeze uses
//   fully. Custom always gets its palette.
void ThemeManager::applyNative(ITheme *theme, const Choice &choice)
{
    QApplication::setStyle(QStyleFactory::create(m_nativeStyle));
    // a style that can't take our themes: the platform's look, whichever theme is chosen
    // (kept for Fusion)
    QString id = theme->id();
    if (!nativeAbilities().themes)
    {
        id = "system";
    }
    bool platformPalette = true;
    if (id == "system")
    {
        TitleBar::setApplicationAppearance(TitleBar::Appearance::System);
    }
    else if (id == "bright" || id == "dark")
    {
        auto appearance = id == "dark" ? TitleBar::Appearance::Dark : TitleBar::Appearance::Bright;
        platformPalette = TitleBar::setApplicationAppearance(appearance);
    }
    else
    {
        const bool dark = Colors::isDark(theme->palette().color(QPalette::Window));
        TitleBar::setApplicationAppearance(dark ? TitleBar::Appearance::Dark : TitleBar::Appearance::Bright);
        platformPalette = false;
    }
    if (!platformPalette)
    {
        const QPalette ours = finished(theme, choice);
        if (m_nativeStyle.toLower() == "breeze")
        {
            // named by what it holds: KWin colours the title bars from the scheme the window
            // names, and reads it again only when that name changes
            const QByteArray contents = kdeColorScheme(ours);
            const QString hash = QCryptographicHash::hash(contents, QCryptographicHash::Sha1).toHex().left(12);
            QDir cache("cache");
            const QString scheme = cache.absoluteFilePath(QString("breeze-%1.colors").arg(hash));
            try
            {
                FS::write(scheme, contents);
                qApp->setProperty(KDE_SCHEME_PROPERTY, scheme);
                // the ones written before
                for (auto &old : cache.entryInfoList({"breeze*.colors"}, QDir::Files))
                {
                    if (old.absoluteFilePath() != scheme)
                        QFile::remove(old.absoluteFilePath());
                }
            }
            catch (const Exception &e)
            {
                qWarning() << "Couldn't write the colour scheme for Breeze:" << e.cause();
            }
        }
        setOurPalette(ours);
        return;
    }
    qApp->setProperty(KDE_SCHEME_PROPERTY, QVariant());
    // With no roles set, Qt (5.15) also gives the platform's palettes for widget classes back
    // (QApplicationPrivate::initializeWidgetPalettesFromTheme): on macOS they hold the native
    // controls' right colours (buttons, item views, labels...).
    // Older Qt has no such thing: an "empty" palette is a copy of the current one. There, the
    // palettes the platform gave at the start.
    if (m_paletteResetWorks)
    {
        QApplication::setPalette(QPalette());
    }
    else
    {
        QApplication::setPalette(m_platformPalette);
        for (const auto &c : m_platformClassPalettes)
        {
            QApplication::setPalette(c.second, c.first);
        }
    }
    // The platform palette has the system's accent already. A custom one goes on top as the
    // only roles set, so everything else keeps following the platform. Setting an application
    // palette with roles clears the class palettes for good, though (and theirs would hide the
    // accent: the item views' has its own Highlight), so they're taken first and put back
    // with the accent in them.
    if (choice.accentSource == AccentSource::Custom && customAccentAvailable(choice.style))
    {
        const Colors::Accent accent = accentFor(choice);
        const QPalette app = QApplication::palette();
        QList<QPair<const char *, QPalette>> classes;
        for (const char *name : THEMED_CLASSES)
        {
            const QPalette palette = QApplication::palette(name);
            if (!(palette == app))
                classes.append(qMakePair(name, palette));
        }
        QApplication::setPalette(accentOnPlatform(app, accent));
        for (const auto &c : classes)
            QApplication::setPalette(accentOnPlatform(c.second, accent, c.second), c.first);
    }
}

ThemeManager::NativeAbilities ThemeManager::nativeAbilities() const
{
    const QString style = m_nativeStyle.toLower();
    // palette-driven, like Fusion
    if (style == "breeze" || style == "kvantum")
        return {true, true, true};
    // Bright and Dark force the app's appearance; checkboxes, radio buttons and default
    // buttons are drawn with AppKit's system accent whatever the palette says, so a custom one
    // would only reach selections and links
    if (style == "macintosh")
        return {true, true, false};
    // anything else: we don't know what it does with a palette; the platform's look as it is
    return {false, false, false};
}

bool ThemeManager::nativeByDefault() const
{
    // KDE-like desktops' own styles, which give us everything
    const QString style = m_nativeStyle.toLower();
    return style == "breeze" || style == "kvantum";
}

bool ThemeManager::hasNativeStyleChoice() const
{
    // Not on Windows (decided 2026-11-07): windowsvista is light only in Qt 5, ignores our
    // accents, and Windows has no single look of its own to match anyway
    return !m_nativeStyle.isEmpty() && m_nativeStyle.toLower() != "fusion"
        && QGuiApplication::platformName() != "windows";
}

QString ThemeManager::nativeStyleName() const
{
    return m_nativeStyle;
}

bool ThemeManager::themeChoiceAvailable(Style style) const
{
    return style != Style::Native || nativeAbilities().themes;
}

bool ThemeManager::accentChoiceAvailable(Style style) const
{
    return style != Style::Native || nativeAbilities().accent;
}

bool ThemeManager::customAccentAvailable(Style style) const
{
    return style != Style::Native || nativeAbilities().customAccent;
}

bool ThemeManager::canFollowSystem() const
{
    return m_system->canFollow();
}

Colors::Accent ThemeManager::systemAccent() const
{
    return m_system->accent();
}
