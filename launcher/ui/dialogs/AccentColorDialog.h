#pragma once

#include <QDialog>

#include <functional>

class QLabel;
class QLineEdit;
class AccentPreview;
class HueStrip;
class SaturationValueField;

/**
 * Picking the custom accent colour: suggested colours (the system's accent among them), a
 * saturation and brightness field with a hue strip, and the colour as hex; a preview of the
 * theme shows it as it would be used. Ours rather than QColorDialog, which on macOS opens a
 * panel in a corner of the screen or picks from the screen through permission prompts.
 */
class AccentColorDialog : public QDialog
{
    Q_OBJECT
public:
    /// preview gives the theme's palette with a colour as its accent
    AccentColorDialog(const QColor &initial, const QColor &systemAccent, std::function<QPalette(const QColor &)> preview,
                      QWidget *parent = nullptr);
    virtual ~AccentColorDialog() {};

    QColor color() const;

private:
    // from anything but the hue strip and the field: they keep their own hue for greys
    void setColor(const QColor &color);
    void colorChanged(bool updateHex);

private: /* data */
    QColor m_color;
    std::function<QPalette(const QColor &)> m_preview;
    AccentPreview *m_previewWidget = nullptr;
    SaturationValueField *m_field = nullptr;
    HueStrip *m_hue = nullptr;
    QLineEdit *m_hex = nullptr;
};
