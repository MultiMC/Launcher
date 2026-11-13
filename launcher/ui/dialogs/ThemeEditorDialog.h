#pragma once

#include <QDialog>
#include <QMap>
#include <QVector>

#include <functional>

#include "ui/themes/ThemeWriter.h"

class ConsolePreview;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QToolButton;
class ThemeManager;
class ThemeSampleWidget;

/**
 * Making a colour theme one's own: its name, its colours and its log colours. Each tab has
 * what it colours next to it, live: Widgets, every colour, with real widgets in them; Console,
 * the log colours, with the console's lines. A colour shows as it is being picked, and every
 * change can be undone. Quick setup sets every colour from three (background, text, accent;
 * ThemeWriter::quickColors), as one change. Save writes the user theme being edited
 * (ThemeWriter::save), which the theme list then reloads and, if it is the current one,
 * applies; Save as makes a new one from any theme, built-in ones too, asking for its name. The stylesheet isn't edited here: Open folder for that.
 */
class ThemeEditorDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * id: the user theme to edit, or empty for a built-in one (only saved as a new one).
     * copyFrom: the edited theme's folder, which a new one is a copy of (stylesheet and
     * images); empty for a built-in one. reserved: ids a new theme can't have (the built-in
     * ones).
     */
    ThemeEditorDialog(const QString &themesFolder, const QString &id, const QString &copyFrom, const ThemeWriter::Theme &initial,
                      const QStringList &reserved, QWidget *parent = nullptr);
    virtual ~ThemeEditorDialog() {};

    /// The theme's id once saved
    QString savedId() const;

    void reject() override;

private:
    QWidget *makeWidgetsTab();
    QWidget *makeConsoleTab();
    QToolButton *colorButton(QWidget *parent);
    void showColor(QToolButton *button, const QColor &color, const QString &unset = QString());
    /**
     * A colour picked in our picker over parent. While it is picked, the samples show tried()
     * of it, the theme with it; whatever comes of it, the theme as it is shows again after.
     * Invalid: cancelled.
     */
    QColor pick(QWidget *parent, const QColor &initial, const QString &title,
                std::function<ThemeWriter::Theme(const QColor &)> tried);
    /// A dialog of the editor's, next to the samples rather than over them
    void placeAside(QWidget *dialog);
    /// Every colour from three, in a dialog with a preview of them; one change
    void quickSetup();
    /// The palette of colors, finished as it is used (the disabled colours derived)
    QPalette paletteOf(const QMap<QPalette::ColorRole, QColor> &colors) const;
    /// What the tabs show next to them, for a theme: the one edited, or one being tried
    void showSamples(const ThemeWriter::Theme &theme);
    /// The console's own colour for a log level, on the theme as it is
    QColor defaultLogColor(const QString &level) const;
    /// After m_theme changed: an undo step, and everything shown again
    void changed();
    void undo();
    void redo();
    void updateControls();
    /// The user theme edited, where it is
    void save();
    /// A new theme, with a name asked for: from any theme
    void saveAs();
    void write(const QString &id, ThemeWriter::Theme theme, const QString &copyFrom);

private: /* data */
    ThemeManager *m_manager = nullptr;
    QString m_folder;
    QString m_id;
    QString m_copyFrom;
    QStringList m_reserved;
    ThemeWriter::Theme m_theme;
    QString m_savedId;
    bool m_modified = false;
    // the theme after each change, the first as it came; m_step is the one shown
    QVector<ThemeWriter::Theme> m_history;
    int m_step = 0;

    QLineEdit *m_name = nullptr;
    QMap<QPalette::ColorRole, QToolButton *> m_roleButtons;
    QMap<QString, QToolButton *> m_logButtons;
    QMap<QString, QToolButton *> m_logResets;
    QStackedWidget *m_samples = nullptr;
    ThemeSampleWidget *m_widgetSample = nullptr;
    ConsolePreview *m_console = nullptr;
    QPushButton *m_undo = nullptr;
    QPushButton *m_redo = nullptr;
    QPushButton *m_save = nullptr;
};
