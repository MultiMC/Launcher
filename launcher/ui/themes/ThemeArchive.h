#pragma once

#include <QString>

#include <nonstd/optional>

/**
 * Themes as zip files, in and out: a colour theme (theme.json) or an icon theme (index.theme),
 * at the top of the zip or in one folder, the layout Prism Launcher's themes use too. Imports
 * are checked before anything lands in the themes' folder: every entry stays inside it, at
 * most MAX_SIZE unpacked, and a colour theme's theme.json reads.
 */
namespace ThemeArchive
{
// themes can carry images, but nothing near this (Design/Theming.md, 2026-11-07)
const qint64 MAX_SIZE = 50 * 1024 * 1024;

enum class Kind
{
    Colors, // goes to themes/
    Icons   // goes to iconthemes/
};

struct Contents
{
    Kind kind;
    QString subdir; // where the theme is in the zip: empty, or the one folder with a slash
    QString id;     // the folder it becomes: that folder's name, or the zip's
};

/// What the zip holds; nothing, with the reason in error, when it isn't a theme
nonstd::optional<Contents> inspect(const QString &zipPath, QString &error);

/// Unpacks the theme into folder/<id>, replacing what is there; false, with the reason in error, when it can't
bool install(const QString &zipPath, const Contents &contents, const QString &folder, QString &error);

/// The theme's folder as a zip, in a folder of its name: the layout install takes
bool exportTo(const QString &themeFolder, const QString &zipPath, QString &error);
}
