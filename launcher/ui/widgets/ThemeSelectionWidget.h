#pragma once

#include <QWidget>

#include "ui/themes/ThemeManager.h"

class QButtonGroup;
class QGroupBox;
class QLabel;
class QRadioButton;
class QToolButton;
class ThemeCard;

/**
 * Choosing the theme, accent and widget style (ThemeManager): a card for each theme with a
 * small window painted in its colours, the accent below (the theme's own, the system's, or a
 * custom colour), and Fusion or Native where Native is offered. Choices a style can't take
 * are disabled, not lost. Shared by the Appearance settings page and the setup wizard.
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
    // the user changed something
    void choiceChanged(const ThemeManager::Choice &choice);

protected:
    void changeEvent(QEvent *event) override;

private:
    void selectTheme(const QString &id);
    void pickCustomAccent();
    void userChanged();
    void updateControls();
    void retranslate();

private: /* data */
    ThemeManager *m_manager = nullptr;
    ThemeManager::Choice m_choice;
    QList<ThemeCard *> m_cards;
    QLabel *m_themeNote = nullptr;
    QGroupBox *m_accentBox = nullptr;
    QButtonGroup *m_accentGroup = nullptr;
    QRadioButton *m_themeAccent = nullptr;
    QRadioButton *m_systemAccent = nullptr;
    QRadioButton *m_customAccent = nullptr;
    QToolButton *m_customColor = nullptr;
    QGroupBox *m_styleBox = nullptr;
    QButtonGroup *m_styleGroup = nullptr;
    QRadioButton *m_fusion = nullptr;
    QRadioButton *m_native = nullptr;
    QLabel *m_styleDescription = nullptr;
};
