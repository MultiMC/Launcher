#pragma once

#include <QModelIndex>
#include <QWidget>

#include "ui/themes/ThemeManager.h"

class QButtonGroup;
class QLabel;
class QListView;
class QRadioButton;
class QToolButton;
class ThemeList;

/**
 * Choosing the theme, accent and widget style (ThemeManager): the style (Fusion or Native,
 * where Native is offered) and the accent (the theme's own, the system's, or a custom colour)
 * on top, the themes below, each with a small window in its colours, who made it and its
 * license. Choices a style can't take are disabled, not lost. Shared by the Appearance
 * settings page and the setup wizard.
 */
class ThemeSelectionWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ThemeSelectionWidget(QWidget *parent = nullptr);
    virtual ~ThemeSelectionWidget() {};

    ThemeManager::Choice choice() const;
    // without emitting choiceChanged
    void setChoice(const ThemeManager::Choice &choice);

signals:
    // a right click on a row (any row, also those that can't be chosen), at globalPos
    void rowMenuRequested(const QModelIndex &index, const QPoint &globalPos);
    // the user changed something
    void choiceChanged(const ThemeManager::Choice &choice);

protected:
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    bool themeSelectable(const QString &id) const;
    void selectTheme(const QString &id);
    void pickCustomAccent();
    void userChanged();
    void updateControls();
    void retranslate();

private: /* data */
    ThemeManager *m_manager = nullptr;
    ThemeManager::Choice m_choice;
    ThemeList *m_list = nullptr;
    QListView *m_view = nullptr;
    bool m_updating = false;
    QLabel *m_themeNote = nullptr;
    QLabel *m_styleLabel = nullptr;
    QWidget *m_styleRow = nullptr;
    QButtonGroup *m_styleGroup = nullptr;
    QRadioButton *m_fusion = nullptr;
    QRadioButton *m_native = nullptr;
    QLabel *m_accentLabel = nullptr;
    QWidget *m_accentRow = nullptr;
    QButtonGroup *m_accentGroup = nullptr;
    QRadioButton *m_themeAccent = nullptr;
    QRadioButton *m_systemAccent = nullptr;
    QRadioButton *m_customAccent = nullptr;
    QToolButton *m_customColor = nullptr;
};
