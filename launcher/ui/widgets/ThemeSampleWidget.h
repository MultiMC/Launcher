#pragma once

#include <QPalette>
#include <QWidget>

class QLabel;

/**
 * Real widgets in a palette, for the theme editor: what the launcher's windows are made of
 * (a list with a selection, buttons, fields, check and radio boxes, a progress bar, tabs, a
 * link, a tooltip), enabled and disabled, so each colour shows where it is used. In the
 * application's style and stylesheet; the palette is set on every widget, because widgets
 * under a stylesheet don't take their parent's.
 */
class ThemeSampleWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ThemeSampleWidget(QWidget *parent = nullptr);
    virtual ~ThemeSampleWidget() {};

    void setSamplePalette(const QPalette &palette);

private:
    void setLinkText(const QPalette &palette);

private: /* data */
    QLabel *m_link = nullptr;
};
