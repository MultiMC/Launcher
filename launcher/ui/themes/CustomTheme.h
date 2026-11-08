#pragma once

#include "ITheme.h"

#include <QMap>

#include "ReuseInfo.h"

/**
 * A theme from a folder in themes/ in the data folder: theme.json (name, colours, log colours,
 * the stylesheet's name), the stylesheet (themeStyle.css by default) and resources/. The same
 * format as MultiMC's custom themes always had, and Prism Launcher's (Design/Theming.md in the
 * MultiMC repository). Read again with reload(); a broken one keeps the reason in error().
 */
class CustomTheme: public ITheme
{
public:
    /// The folder's name is the theme's id
    explicit CustomTheme(const QString &folder);
    virtual ~CustomTheme() {}

    QString id() override;
    QString name() override;
    QPalette palette() override;
    QString styleSheet() override;
    QStringList searchPaths() override;
    QMap<QString, QColor> logColors() override;

    /// Read the folder again
    void reload();
    /// Why the theme can't be used; empty when it can
    QString error() const;
    /// What older versions left in themes/custom on every start: not offered
    bool isOldDefault() const;
    /// The files the theme is read from, to watch for changes
    QStringList files() const;
    /// Who made it and its license, from theme.json.license (the community themes have one)
    ReuseInfo reuse() const;
    QString folder() const;

    /// The palette roles theme.json can set, by their names there
    static QList<QPair<QPalette::ColorRole, QString>> colorRoles();
    /// The log levels theme.json can colour, by their names there
    static QStringList logLevels();
    /// Every role set from the ones given: the rest from Bright or Dark, whichever is closer
    static QPalette paletteFrom(const QMap<QPalette::ColorRole, QColor> &given);

private: /* data */
    QString m_folder;
    QString m_id;
    QString m_name;
    QString m_error;
    QPalette m_palette;
    QString m_styleSheet;
    QString m_styleSheetPath;
    QMap<QString, QColor> m_logColors;
    bool m_oldDefault = false;
    ReuseInfo m_reuse;
};
