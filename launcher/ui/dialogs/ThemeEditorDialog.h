#pragma once

#include <QDialog>
#include <QMap>

#include <functional>

#include "ui/themes/ThemeWriter.h"

class ColorEdit;
class ConsolePreview;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QToolButton;
class QUndoStack;
class ThemeManager;
class ThemeSampleWidget;

/**
 * Making a colour theme one's own: its name, its colours and its log colours. Each tab has
 * what it colours next to it, live: Widgets, every colour, with real widgets in them; Console,
 * the log colours, with the console's lines. A colour shows as it is being picked or typed.
 * Every change, the name's typing too, is a step in one history (QUndoStack): undone and
 * redone with the buttons, their menus of steps, and the usual keys, also in the text fields,
 * which have no undo of their own. Quick setup sets every colour from three (background, text, accent;
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

    /// The theme as a step of the history has it, shown (ThemeChange)
    void setTheme(const ThemeWriter::Theme &theme);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QWidget *makeWidgetsTab();
    QWidget *makeConsoleTab();
    /**
     * A colour to type or pick (title: the picker's), current() its colour now: while typed or
     * picked, the samples show tried() of it; set() puts it in m_theme, as one change
     */
    ColorEdit *colorEdit(QWidget *parent, const QString &title, std::function<QColor()> current,
                         std::function<ThemeWriter::Theme(const QColor &)> tried, std::function<void(const QColor &)> set);
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
    /// After m_theme changed: an undo step (text: what changed; merged with the one before it
    /// of the same mergeId), and everything shown again
    void changed(const QString &text, int mergeId = -1);
    void undo();
    void redo();
    /// The Undo or Redo button: a step, or from its menu, any number
    QToolButton *historyButton(const QString &text, bool undo);
    void updateHistoryButtons();
    /// A colour being typed and not set yet goes, before the history moves; false: none was
    bool settleTyping();
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
    // the theme as it came, and as the history last showed it (where a change starts from)
    ThemeWriter::Theme m_original;
    ThemeWriter::Theme m_applied;
    QUndoStack *m_undoStack = nullptr;

    QLineEdit *m_name = nullptr;
    QMap<QPalette::ColorRole, ColorEdit *> m_roleEdits;
    QMap<QString, ColorEdit *> m_logEdits;
    QMap<QString, QToolButton *> m_logResets;
    QStackedWidget *m_samples = nullptr;
    ThemeSampleWidget *m_widgetSample = nullptr;
    ConsolePreview *m_console = nullptr;
    QToolButton *m_undo = nullptr;
    QToolButton *m_redo = nullptr;
    QPushButton *m_save = nullptr;
};
