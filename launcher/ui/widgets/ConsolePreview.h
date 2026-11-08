#pragma once

#include <QColor>
#include <QMap>
#include <QTextEdit>

/**
 * A few lines as the console shows them, one per kind of message, in a theme's log colours
 * (ITheme::logColors) on this widget's own palette, in the console's font. For the console
 * settings and the theme editor.
 */
class ConsolePreview : public QTextEdit
{
    Q_OBJECT
public:
    explicit ConsolePreview(QWidget *parent = nullptr);
    virtual ~ConsolePreview() {};

    void setConsoleFont(const QString &family, int size);
    /// by level name; the missing ones are the log's own
    void setLogColors(const QMap<QString, QColor> &colors);
    /// The lines again: after the palette changed
    void refresh();

protected:
    void changeEvent(QEvent *event) override;

private: /* data */
    QString m_family;
    int m_size = 11;
    QMap<QString, QColor> m_logColors;
};
