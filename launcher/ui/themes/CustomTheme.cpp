#include "CustomTheme.h"
#include "BrightTheme.h"
#include "DarkTheme.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <Json.h>
#include <FileSystem.h>

namespace {
const char *THEME_FILE = "theme.json";
const char *DEFAULT_STYLE_FILE = "themeStyle.css";

// the colours theme.json can set, by their names there
const QList<QPair<QPalette::ColorRole, QString>> ROLES = {
    {QPalette::Window, "Window"},
    {QPalette::WindowText, "WindowText"},
    {QPalette::Base, "Base"},
    {QPalette::AlternateBase, "AlternateBase"},
    {QPalette::ToolTipBase, "ToolTipBase"},
    {QPalette::ToolTipText, "ToolTipText"},
    {QPalette::Text, "Text"},
    {QPalette::Button, "Button"},
    {QPalette::ButtonText, "ButtonText"},
    {QPalette::BrightText, "BrightText"},
    {QPalette::Link, "Link"},
    {QPalette::Highlight, "Highlight"},
    {QPalette::HighlightedText, "HighlightedText"},
};

// the log levels theme.json can colour, by their names there (Prism Launcher's)
const QStringList LOG_LEVELS = {"Launcher", "Message", "Debug", "Warning", "Error", "Fatal", "FatalHighlight"};

// What older versions wrote to themes/custom on every start, whether anyone wanted a custom
// theme or not: the Dark theme of the time and its tooltip stylesheet. Not offered.
const QMap<QPalette::ColorRole, QColor> OLD_DEFAULT_COLORS = {
    {QPalette::Window, QColor("#31363b")},
    {QPalette::WindowText, QColor("#ffffff")},
    {QPalette::Base, QColor("#232629")},
    {QPalette::AlternateBase, QColor("#31363b")},
    {QPalette::ToolTipBase, QColor("#ffffff")},
    {QPalette::ToolTipText, QColor("#ffffff")},
    {QPalette::Text, QColor("#ffffff")},
    {QPalette::Button, QColor("#31363b")},
    {QPalette::ButtonText, QColor("#ffffff")},
    {QPalette::BrightText, QColor("#ff0000")},
    {QPalette::Link, QColor("#2a82da")},
    {QPalette::Highlight, QColor("#2a82da")},
    {QPalette::HighlightedText, QColor("#000000")},
};
const char *OLD_DEFAULT_CSS = "QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }";

QColor readColor(const QJsonObject &object, const QString &key, const QString &themeId)
{
    auto value = Json::ensureString(object, key, QString());
    if(value.isEmpty())
    {
        return QColor();
    }
    QColor color(value);
    if(!color.isValid())
    {
        qWarning() << "Theme" << themeId << "- color value" << value << "for" << key << "was not recognized.";
    }
    return color;
}

// Every role set: the colours given over Bright's or Dark's, whichever is closer to the
// theme's window colour, and the shading roles derived from the Button and Window colours
QPalette fullPalette(const QMap<QPalette::ColorRole, QColor> &given)
{
    const QColor window = given.value(QPalette::Window);
    const QPalette base = window.isValid() && !Colors::isDark(window) ? BrightTheme::colors() : DarkTheme::colors();
    auto color = [&](QPalette::ColorRole role) { return given.value(role, base.color(role)); };
    QPalette palette(color(QPalette::Button), color(QPalette::Window));
    for(auto &role: ROLES)
    {
        palette.setColor(role.first, color(role.first));
    }
    palette.setColor(QPalette::LinkVisited, given.contains(QPalette::Link) ? color(QPalette::Link) : base.color(QPalette::LinkVisited));
    return palette;
}
}

CustomTheme::CustomTheme(const QString &folder) : m_folder(folder)
{
    m_id = QFileInfo(folder).fileName();
    reload();
}

void CustomTheme::reload()
{
    m_name = m_id;
    m_error.clear();
    m_palette = DarkTheme::colors();
    m_styleSheet.clear();
    m_styleSheetPath = QDir(m_folder).absoluteFilePath(DEFAULT_STYLE_FILE);
    m_logColors.clear();
    m_oldDefault = false;

    const QString themeFile = QDir(m_folder).absoluteFilePath(THEME_FILE);
    m_reuse = ReuseInfo::read(themeFile);
    QMap<QPalette::ColorRole, QColor> colors;
    try
    {
        auto doc = Json::requireDocument(themeFile, "Theme JSON file");
        const QJsonObject root = Json::requireObject(doc, "Theme JSON file");
        m_name = Json::ensureString(root, "name", m_id);
        // the widget style is the one chosen in the settings, and disabled colours follow
        // from the others (Design/Theming.md in the MultiMC repository)
        if(root.contains("widgets"))
        {
            qDebug() << "Theme" << m_id << "chooses a widget style; ignored, the one from the settings is used.";
        }
        auto colorsRoot = Json::ensureObject(root, "colors");
        if(colorsRoot.contains("fadeColor") || colorsRoot.contains("fadeAmount"))
        {
            qDebug() << "Theme" << m_id << "sets fadeColor or fadeAmount; ignored, disabled colours follow from the others.";
        }
        for(auto &role: ROLES)
        {
            auto color = readColor(colorsRoot, role.second, m_id);
            if(color.isValid())
            {
                colors[role.first] = color;
            }
        }
        auto logRoot = Json::ensureObject(root, "logColors");
        for(auto &level: LOG_LEVELS)
        {
            auto color = readColor(logRoot, level, m_id);
            if(color.isValid())
            {
                m_logColors[level] = color;
            }
        }
        // the stylesheet stays inside the theme's folder
        auto qssFile = Json::ensureString(root, "qssFilePath", DEFAULT_STYLE_FILE);
        auto styleSheetPath = FS::PathInside(m_folder, qssFile);
        if(!styleSheetPath)
        {
            m_error = QObject::tr("Its stylesheet (%1) is outside of its folder.").arg(qssFile);
            return;
        }
        m_styleSheetPath = *styleSheetPath;
    }
    catch (const Exception &e)
    {
        qWarning() << "Couldn't load theme" << m_id << ":" << e.cause();
        m_error = e.cause();
        return;
    }
    m_palette = fullPalette(colors);

    if(QFileInfo(m_styleSheetPath).isFile())
    {
        try
        {
            m_styleSheet = QString::fromUtf8(FS::read(m_styleSheetPath));
        }
        catch (const Exception &e)
        {
            qWarning() << "Couldn't load the stylesheet of theme" << m_id << ":" << e.cause();
            m_error = e.cause();
            return;
        }
    }
    const QString css = m_styleSheet.trimmed();
    m_oldDefault = m_id == "custom" && colors == OLD_DEFAULT_COLORS && (css.isEmpty() || css == OLD_DEFAULT_CSS);
}

QString CustomTheme::folder() const
{
    return m_folder;
}

QList<QPair<QPalette::ColorRole, QString>> CustomTheme::colorRoles()
{
    return ROLES;
}

QStringList CustomTheme::logLevels()
{
    return LOG_LEVELS;
}

QPalette CustomTheme::paletteFrom(const QMap<QPalette::ColorRole, QColor> &given)
{
    return fullPalette(given);
}

QString CustomTheme::error() const
{
    return m_error;
}

ReuseInfo CustomTheme::reuse() const
{
    return m_reuse;
}

bool CustomTheme::isOldDefault() const
{
    return m_oldDefault;
}

QStringList CustomTheme::files() const
{
    return {QDir(m_folder).absoluteFilePath(THEME_FILE), m_styleSheetPath};
}

QStringList CustomTheme::searchPaths()
{
    return { FS::PathCombine(m_folder, "resources") };
}

QString CustomTheme::id()
{
    return m_id;
}

QString CustomTheme::name()
{
    return m_name;
}

QPalette CustomTheme::palette()
{
    return m_palette;
}

QString CustomTheme::styleSheet()
{
    return m_styleSheet;
}

QMap<QString, QColor> CustomTheme::logColors()
{
    return m_logColors;
}
