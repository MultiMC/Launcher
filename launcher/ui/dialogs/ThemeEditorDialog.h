#pragma once

#include <QDialog>
#include <QMap>

#include <functional>

#include "ui/themes/ThemeWriter.h"

class ConsolePreview;
class QDialogButtonBox;
class QLineEdit;
class QToolButton;
class ThemeManager;
class ThemePreviewWidget;

/**
 * Making a colour theme one's own: its name, its colours and its log colours, with a preview
 * of a window and the console in them. The colours two ways: Quick, three colours (background,
 * text, accent) that every other follows from (ThemeWriter::quickColors), and Every color, each
 * on its own to dial it in. Save writes theme.json (ThemeWriter::save): a new theme for a
 * copy, or the theme being edited, which the theme list then reloads and, if it is the current
 * one, applies. The stylesheet isn't edited here: Open folder for that.
 */
class ThemeEditorDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * id: the user theme to edit, or empty for a new one, then copied from copyFrom (a theme's
     * folder) when that isn't empty. reserved: ids a new theme can't have (the built-in ones).
     */
    ThemeEditorDialog(const QString &themesFolder, const QString &id, const QString &copyFrom, const ThemeWriter::Theme &initial,
                      const QStringList &reserved, QWidget *parent = nullptr);
    virtual ~ThemeEditorDialog() {};

    /// The theme's id once saved
    QString savedId() const;

    void reject() override;

private:
    QWidget *makeQuickTab();
    QWidget *makeColorsTab();
    QWidget *makeConsoleTab();
    QToolButton *colorButton(QWidget *parent);
    void showColor(QToolButton *button, const QColor &color, const QString &unset = QString());
    QColor pick(const QColor &initial, const QString &title, std::function<void(QPalette &, const QColor &)> apply);
    QPalette previewPalette() const;
    /// The console's own colour for a log level, on the theme as it is
    QColor defaultLogColor(const QString &level) const;
    void changed();
    void updateControls();
    void save();

private: /* data */
    ThemeManager *m_manager = nullptr;
    QString m_folder;
    QString m_id;
    QString m_copyFrom;
    QStringList m_reserved;
    ThemeWriter::Theme m_theme;
    QString m_savedId;
    bool m_modified = false;

    QLineEdit *m_name = nullptr;
    QToolButton *m_background = nullptr;
    QToolButton *m_text = nullptr;
    QToolButton *m_accent = nullptr;
    QMap<QPalette::ColorRole, QToolButton *> m_roleButtons;
    QMap<QString, QToolButton *> m_logButtons;
    QMap<QString, QToolButton *> m_logResets;
    ThemePreviewWidget *m_preview = nullptr;
    ConsolePreview *m_console = nullptr;
    QDialogButtonBox *m_buttons = nullptr;
};
