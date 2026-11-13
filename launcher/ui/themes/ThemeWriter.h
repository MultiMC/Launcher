#pragma once

#include <QColor>
#include <QMap>
#include <QPalette>
#include <QString>
#include <QStringList>

/**
 * Writing colour themes, for the editor: theme.json with a name, the palette's roles and the
 * log colours (and the accent's and title bar's modes, when given). What else theme.json holds (the stylesheet's name, keys of other launchers)
 * stays as it was; the keys we ignore (widgets, fadeColor, fadeAmount) go. A new theme is
 * written next to the themes' folder and moved in at once, as an import is, so the list never
 * sees half of it.
 */
namespace ThemeWriter
{
struct Theme
{
    QString name;
    QMap<QPalette::ColorRole, QColor> colors;
    QMap<QString, QColor> logColors; // by level name; the missing ones are the log's own
    /// theme.json's accent and titleBar modes (CustomTheme::accentModeName, titleBarModeName);
    /// empty leaves them as they are. With a title bar mode, the title bar's colours are
    /// written too (TitleBar, TitleBarText), or removed when invalid.
    QString accent;
    QString titleBar;
    QColor titleBarColor;
    QColor titleBarTextColor;
};

/**
 * Every role from three colours, for the editor's quick mode: the window's background, its
 * text, and the accent. Fields are a step further from the text than the window (darker on
 * dark themes, lighter on bright ones) and buttons stand a little apart from it; text, links
 * and errors are kept readable on what they are drawn on; selected text is white or black,
 * whichever reads on the accent.
 */
QMap<QPalette::ColorRole, QColor> quickColors(const QColor &background, const QColor &text, const QColor &accent);

/// A folder name in themesFolder for a theme of this name, not in use and not one of reserved
QString freeId(const QString &themesFolder, const QString &name, const QStringList &reserved);

/**
 * Writes the theme into themesFolder/id. If that folder exists, its theme.json is updated in
 * place. Otherwise the folder is made, as a copy of copyFrom (a theme's folder: its stylesheet
 * and resources) when given. False, with the reason in error, when it can't.
 */
bool save(const QString &themesFolder, const QString &id, const Theme &theme, const QString &copyFrom, QString &error);
}
