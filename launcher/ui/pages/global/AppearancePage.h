#pragma once

#include <QModelIndex>
#include <QWidget>

#include "ui/pages/BasePage.h"
#include "ui/themes/ThemeArchive.h"
#include <Application.h>

class ConsolePreview;
class IconThemeSelectionWidget;
class QAbstractItemModel;
class QFontComboBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTabWidget;
class ThemeSelectionWidget;

/**
 * How the launcher looks, in tabs: the theme, accent and style (ThemeSelectionWidget), the
 * icons (IconThemeSelectionWidget), and the console's font, with a preview in the theme's log
 * colours. Choices apply at once. Themes and icon themes come in as zips (Import…, or dropped
 * on the page) and go out as zips (Export…); user themes can be opened in the file manager or
 * deleted, from the buttons for the chosen one or from any row's menu. Colour themes are made
 * one's own in the theme editor (ThemeEditorDialog), opened with Edit… on any theme: it saves
 * a user theme in place, and any theme as a new one (Save as).
 */
class AppearancePage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit AppearancePage(QWidget *parent = 0);
    virtual ~AppearancePage() {};

    QString displayName() const override
    {
        return tr("Appearance");
    }
    QIcon icon() const override
    {
        return APPLICATION->getThemedIcon("resourcepacks");
    }
    QString id() const override
    {
        return "appearance-settings";
    }
    QString helpPage() const override
    {
        return "Launcher-settings";
    }
    void changeEvent(QEvent *) override;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    using Kind = ThemeArchive::Kind;
    // the buttons under a tab's list
    struct FileButtons
    {
        QPushButton *importButton = nullptr;
        QPushButton *openFolder = nullptr;
        QPushButton *edit = nullptr; // colour themes only
        QPushButton *exportButton = nullptr;
        QPushButton *deleteButton = nullptr;
    };

    QWidget *makeListTab(QWidget *list, FileButtons &buttons, Kind kind);
    QWidget *makeConsoleTab();
    void consoleFontChanged();
    void updateConsolePreview();
    void loadSettings();
    void retranslate();

    QAbstractItemModel *model(Kind kind) const;
    QString folder(Kind kind) const;
    QModelIndex indexOf(Kind kind, const QString &id) const;
    QString currentId(Kind kind) const;
    bool isUserTheme(const QModelIndex &index) const;
    void select(Kind kind, const QString &id);
    void updateFileButtons();
    void showRowMenu(Kind kind, const QModelIndex &index, const QPoint &globalPos);
    void importClicked();
    void importZip(const QString &path);
    void exportTheme(Kind kind, const QModelIndex &index);
    void deleteTheme(Kind kind, const QModelIndex &index);
    /// The colour theme in the editor: in place when edit, else a copy
    void editTheme(const QModelIndex &index);

private: /* data */
    QTabWidget *m_tabs = nullptr;
    ThemeSelectionWidget *m_themeWidget = nullptr;
    QWidget *m_themeTab = nullptr;
    FileButtons m_themeButtons;
    IconThemeSelectionWidget *m_iconsWidget = nullptr;
    QWidget *m_iconsTab = nullptr;
    FileButtons m_iconsButtons;
    QWidget *m_consoleTab = nullptr;
    QLabel *m_fontLabel = nullptr;
    QFontComboBox *m_consoleFont = nullptr;
    QSpinBox *m_fontSize = nullptr;
    ConsolePreview *m_consolePreview = nullptr;
    bool m_loading = false;
};
