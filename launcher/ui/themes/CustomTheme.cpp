#include "CustomTheme.h"
#include "DarkTheme.h"
#include <QDir>
#include <Json.h>
#include <FileSystem.h>

const char *themeFile = "theme.json";
const char *styleFile = "themeStyle.css";

namespace {
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

// What older versions wrote here on every start, whether anyone wanted a custom theme or
// not: the Dark theme of the time and its tooltip stylesheet. Files like these aren't offered.
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
}

// The colours theme.json sets, by role
static bool readThemeJson(const QString &path, QMap<QPalette::ColorRole, QColor> &colors, QString &name)
{
    QFileInfo pathInfo(path);
    if(!pathInfo.exists() || !pathInfo.isFile())
    {
        qDebug() << "No theme json present.";
        return false;
    }
    try
    {
        auto doc = Json::requireDocument(path, "Theme JSON file");
        const QJsonObject root = doc.object();
        name = Json::requireString(root, "name", "Theme name");
        // the widget style is the one chosen in the settings, and disabled colours follow
        // from the others (Design/Theming.md in the MultiMC repository)
        if(root.contains("widgets"))
        {
            qDebug() << "Theme" << name << "chooses a widget style; ignored, the one from the settings is used.";
        }
        auto colorsRoot = Json::requireObject(root, "colors", "colors object");
        if(colorsRoot.contains("fadeColor") || colorsRoot.contains("fadeAmount"))
        {
            qDebug() << "Theme" << name << "sets fadeColor or fadeAmount; ignored, disabled colours follow from the others.";
        }
        for(auto &role: ROLES)
        {
            auto colorValue = Json::ensureString(colorsRoot, role.second, QString());
            if(colorValue.isEmpty())
            {
                qDebug() << "Color value for" << role.second << "was not present.";
                continue;
            }
            QColor color(colorValue);
            if(!color.isValid())
            {
                qWarning() << "Color value" << colorValue << "for" << role.second << "was not recognized.";
                continue;
            }
            colors[role.first] = color;
        }
    }
    catch (const Exception &e)
    {
        qWarning() << "Couldn't load theme json: " << e.cause();
        return false;
    }
    return true;
}

// Every role set: the colours given over the Dark theme's, the shading roles derived from the
// Button and Window colours, so a bright theme gets bright ones
static QPalette fullPalette(const QMap<QPalette::ColorRole, QColor> &given)
{
    const QPalette base = DarkTheme::colors();
    auto color = [&](QPalette::ColorRole role) { return given.value(role, base.color(role)); };
    QPalette palette(color(QPalette::Button), color(QPalette::Window));
    for(auto &role: ROLES)
    {
        palette.setColor(role.first, color(role.first));
    }
    palette.setColor(QPalette::LinkVisited, color(QPalette::Link));
    return palette;
}

CustomTheme::CustomTheme(QString folder)
{
    m_id = folder;
    m_name = "Custom";
    m_palette = DarkTheme::colors();
    QString path = FS::PathCombine("themes", m_id);

    QMap<QPalette::ColorRole, QColor> colors;
    if (readThemeJson(FS::PathCombine(path, themeFile), colors, m_name))
    {
        m_palette = fullPalette(colors);
        m_customized = colors != OLD_DEFAULT_COLORS;
    }

    auto cssFilePath = FS::PathCombine(path, styleFile);
    QFileInfo info (cssFilePath);
    if(info.isFile())
    {
        try
        {
            // TODO: validate css?
            m_styleSheet = QString::fromUtf8(FS::read(cssFilePath));
            const QString css = m_styleSheet.trimmed();
            m_customized = m_customized || (!css.isEmpty() && css != OLD_DEFAULT_CSS);
        }
        catch (const Exception &e)
        {
            qWarning() << "Couldn't load css:" << e.cause() << "from" << cssFilePath;
        }
    }
    qDebug() << "Custom theme" << m_id << (m_customized ? "found" : "not customized: not offered");
}

bool CustomTheme::isCustomized() const
{
    return m_customized;
}

QStringList CustomTheme::searchPaths()
{
    return { FS::PathCombine("themes", m_id, "resources") };
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
