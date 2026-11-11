#pragma once

#include <QColor>
#include <QObject>
#include <QPalette>
#include <QMap>
#include <vector>
#include <memory>

#include "Colors.h"
#include "settings/SettingsObject.h"

class ITheme;
class IconThemeList;
class ThemeList;
class SystemAppearance;

/**
 * The widget style, theme and accent (Design/Theming.md in the MultiMC repository): our
 * palettes on Fusion by default, the platform's own style (Native) where it is good, the
 * System theme and accent following the system live. Reads and writes the Theme, ThemeStyle,
 * AccentSource and AccentColor settings.
 */
class ThemeManager : public QObject
{
    Q_OBJECT
public:
    enum class Style
    {
        Fusion, // our palettes on Fusion, the same everywhere
        Native  // the platform's style: macintosh, Breeze, Kvantum, whatever the desktop gives
    };

    enum class AccentSource
    {
        Theme,  // the theme's own (the System theme's is the system's)
        System, // the system's, followed when it changes
        Custom  // one picked by the user
    };

    /// Everything the settings choose
    struct Choice
    {
        QString theme;
        Style style = Style::Fusion;
        AccentSource accentSource = AccentSource::Theme;
        QColor accentColor;

        bool operator==(const Choice &other) const;
        bool operator!=(const Choice &other) const
        {
            return !(*this == other);
        }
    };

    /// Before anything changes the style: remembers the platform's style and palette
    explicit ThemeManager(SettingsObjectPtr settings, QObject *parent = nullptr);
    virtual ~ThemeManager();

    /// What the settings say, with the defaults for what isn't set yet
    Choice savedChoice() const;
    /// A theme's palette finished as with Fusion and the theme's own accent (the disabled
    /// colours derived): for the theme editor's preview
    QPalette finishedPalette(const QPalette &themePalette) const;
    /// The theme when none was chosen, or the chosen one is gone
    QString defaultTheme() const;
    void save(const Choice &choice);
    /// Whether a theme was ever chosen (the setup wizard asks until then)
    bool themeChosen() const;

    /// Set the style, palette, stylesheet and title bars; every window gets a PaletteChange
    void apply(const Choice &choice);
    Choice current() const;

    /// System (where the system can be followed), Bright, Dark, then the ones in themes/
    ThemeList *themeList() const;
    /// The built-in icon sets, then the ones in iconthemes/
    IconThemeList *iconThemeList() const;
    /// The current theme's colours for log levels (see ITheme::logColors)
    QMap<QString, QColor> logColors() const;
    /// The choice's theme palette as it would look with Fusion, accent and all: for previews
    QPalette previewPalette(const QString &themeId, const Choice &choice) const;

    /// Native: offered on macOS and Linux, never on Windows
    bool hasNativeStyleChoice() const;
    QString nativeStyleName() const;
    /// What a style can take: our themes, any accent, a custom accent
    bool themeChoiceAvailable(Style style) const;
    bool accentChoiceAvailable(Style style) const;
    bool customAccentAvailable(Style style) const;

    bool canFollowSystem() const;
    Colors::Accent systemAccent() const;

signals:
    /// The system's dark mode or accent changed (after it was applied, if it matters)
    void systemChanged();
    /// A theme was applied: a choice, or the current theme changed on disk
    void applied();

private:
    struct NativeAbilities
    {
        bool themes;       // Bright, Dark, Custom: our palettes, or a forced appearance
        bool accent;       // an accent choice at all (the system's, at least)
        bool customAccent; // a custom accent
    };
    NativeAbilities nativeAbilities() const;
    bool nativeByDefault() const;
    ITheme *theme(const QString &id) const;
    /// What applies of a choice: a theme that exists, a style that is offered
    Choice effective(Choice choice) const;
    Colors::Accent accentFor(const Choice &choice) const;
    QPalette finished(ITheme *theme, const Choice &choice) const;
    void applyFusion(ITheme *theme, const Choice &choice);
    void applyNative(ITheme *theme, const Choice &choice);
    void setOurPalette(const QPalette &palette);
    void forgetStyleSheetFonts();

private: /* data */
    SettingsObjectPtr m_settings;
    SystemAppearance *m_system = nullptr;
    std::vector<std::unique_ptr<ITheme>> m_builtIn;
    ThemeList *m_themeList = nullptr;
    IconThemeList *m_iconThemeList = nullptr;
    Choice m_current;
    // the platform's style and palette, before we set any
    QString m_nativeStyle;
    QPalette m_platformPalette;
    QList<QPair<const char *, QPalette>> m_platformClassPalettes;
    // whether setting a palette with no roles gives the platform's back (Qt 5.15+)
    bool m_paletteResetWorks = true;
};
