#pragma once

#include <QWidget>

class IconCard;

/**
 * Choosing the icon theme: a card for each set with a few of its icons. Custom (icons in
 * iconthemes/custom in the data folder) only where it exists. Shared by the Appearance
 * settings page and the setup wizard.
 */
class IconThemeSelectionWidget : public QWidget
{
    Q_OBJECT
public:
    explicit IconThemeSelectionWidget(QWidget *parent = nullptr);
    virtual ~IconThemeSelectionWidget() {};

    QString iconTheme() const;
    // without emitting iconThemeChanged
    void setIconTheme(const QString &id);

signals:
    // the user chose another set
    void iconThemeChanged(const QString &id);

protected:
    void changeEvent(QEvent *event) override;

private:
    void select(const QString &id);
    void retranslate();

private: /* data */
    QList<IconCard *> m_cards;
    QString m_iconTheme;
};
