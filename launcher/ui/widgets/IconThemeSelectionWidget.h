#pragma once

#include <QModelIndex>
#include <QWidget>

class IconThemeList;
class QListView;

/**
 * Choosing the icon theme: a row for each set (IconThemeList), built-in and in iconthemes/,
 * with a few of its icons, its description, who made it and its license. Shared by the
 * Appearance settings page and the setup wizard.
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
    // a right click on a row (any row, also those that can't be chosen), at globalPos
    void rowMenuRequested(const QModelIndex &index, const QPoint &globalPos);
    // the user chose another set
    void iconThemeChanged(const QString &id);

protected:
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void showSelection();

private: /* data */
    IconThemeList *m_list = nullptr;
    QListView *m_view = nullptr;
    QString m_iconTheme;
    bool m_updating = false;
};
