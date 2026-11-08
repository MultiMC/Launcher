#include "ThemeWriter.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include "Colors.h"
#include "CustomTheme.h"
#include "Exception.h"
#include "FileSystem.h"
#include "Json.h"

namespace {
const char *THEME_FILE = "theme.json";

QString tr(const char *text)
{
    return QCoreApplication::translate("ThemeWriter", text);
}

// theme.json in folder with the theme's name and colours, over what it held before
bool writeThemeFile(const QString &folder, const ThemeWriter::Theme &theme, QString &error)
{
    const QString path = QDir(folder).absoluteFilePath(THEME_FILE);
    QJsonObject root;
    if (QFileInfo(path).isFile())
    {
        try
        {
            root = Json::requireObject(Json::requireDocument(path, "Theme JSON file"), "Theme JSON file");
        }
        catch (const Exception &e)
        {
            // it is written anew: what it held can't be read anyway
            qWarning() << "Replacing unreadable" << path << ":" << e.cause();
        }
    }
    root["name"] = theme.name;
    // the widget style and the disabled colours are ours to choose (CustomTheme)
    root.remove("widgets");

    QJsonObject colors = root.value("colors").toObject();
    colors.remove("fadeColor");
    colors.remove("fadeAmount");
    for (auto &role : CustomTheme::colorRoles())
    {
        const QColor color = theme.colors.value(role.first);
        if (color.isValid())
            colors[role.second] = color.name();
        else
            colors.remove(role.second);
    }
    root["colors"] = colors;

    QJsonObject logColors;
    for (auto &level : CustomTheme::logLevels())
    {
        const QColor color = theme.logColors.value(level);
        if (color.isValid())
            logColors[level] = color.name();
    }
    if (logColors.isEmpty())
        root.remove("logColors");
    else
        root["logColors"] = logColors;

    try
    {
        FS::write(path, QJsonDocument(root).toJson(QJsonDocument::Indented));
    }
    catch (const Exception &e)
    {
        error = tr("The theme file %1 can't be written: %2").arg(path, e.cause());
        return false;
    }
    return true;
}
}

namespace ThemeWriter
{
QMap<QPalette::ColorRole, QColor> quickColors(const QColor &background, const QColor &text, const QColor &accent)
{
    const bool dark = Colors::isDark(background);
    const QColor base = dark ? Colors::mix(background, Qt::black, 0.25) : Colors::mix(background, Qt::white, 0.7);
    const QColor button = dark ? Colors::mix(background, text, 0.06) : Colors::mix(background, Qt::white, 0.6);
    const QColor readableText = Colors::readable(text, base);
    QMap<QPalette::ColorRole, QColor> out;
    out[QPalette::Window] = background;
    out[QPalette::WindowText] = Colors::readable(text, background);
    out[QPalette::Base] = base;
    out[QPalette::AlternateBase] = Colors::mix(base, background, 0.5);
    out[QPalette::Text] = readableText;
    out[QPalette::Button] = button;
    out[QPalette::ButtonText] = Colors::readable(text, button);
    out[QPalette::ToolTipBase] = base;
    out[QPalette::ToolTipText] = readableText;
    out[QPalette::BrightText] = Colors::readable(QColor("#e81123"), base);
    out[QPalette::Link] = Colors::readable(accent, base);
    out[QPalette::Highlight] = accent;
    out[QPalette::HighlightedText] = Colors::isDark(accent) ? QColor(Qt::white) : QColor(Qt::black);
    return out;
}

QString freeId(const QString &themesFolder, const QString &name, const QStringList &reserved)
{
    QString base = FS::RemoveInvalidFilenameChars(name.trimmed(), '-');
    // hidden folders aren't themes
    while (base.startsWith('.'))
        base.remove(0, 1);
    if (base.isEmpty())
        base = "theme";
    QString id = base;
    for (int n = 2; QFileInfo::exists(QDir(themesFolder).absoluteFilePath(id)) || reserved.contains(id, Qt::CaseInsensitive); n++)
    {
        id = QString("%1 %2").arg(base).arg(n);
    }
    return id;
}

bool save(const QString &themesFolder, const QString &id, const Theme &theme, const QString &copyFrom, QString &error)
{
    const QString root = QDir(themesFolder).absolutePath();
    const auto target = FS::PathInside(root, id);
    if (!target || *target == root || id.isEmpty())
    {
        error = tr("The theme's folder name \"%1\" can't be used.").arg(id);
        return false;
    }
    if (QFileInfo(*target).isDir())
    {
        return writeThemeFile(*target, theme, error);
    }

    if (!QDir().mkpath(root))
    {
        error = tr("The folder %1 can't be created.").arg(root);
        return false;
    }
    // made next to the themes, out of their list's sight, then moved in at once
    QTemporaryDir temp(QDir(QFileInfo(root).absolutePath()).absoluteFilePath(".theme-new-XXXXXX"));
    if (!temp.isValid())
    {
        error = tr("A temporary folder can't be created.");
        return false;
    }
    const QString staged = QDir(temp.path()).absoluteFilePath(id);
    if (!copyFrom.isEmpty())
    {
        FS::copy copy(copyFrom, staged);
        if (!copy())
        {
            error = tr("The theme in %1 can't be copied.").arg(copyFrom);
            return false;
        }
    }
    else if (!QDir().mkpath(staged))
    {
        error = tr("The folder %1 can't be created.").arg(staged);
        return false;
    }
    if (!writeThemeFile(staged, theme, error))
    {
        return false;
    }
    if (!QDir().rename(staged, *target))
    {
        error = tr("The theme can't be moved to %1.").arg(*target);
        return false;
    }
    return true;
}
}
