#include "AccentColorDialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRegExpValidator>
#include <QToolButton>
#include <QVBoxLayout>

#include "Colors.h"
#include "ui/widgets/ThemePreview.h"

namespace {
// Suggestions: Windows' accent colours, which work as selections on bright and dark themes
const char *const SUGGESTED[] = {"#0078d4", "#8764b8", "#e3008c", "#d13438", "#f7630c",
                                 "#ffb900", "#10893e", "#00b7c3", "#69797e"};

QIcon swatch(const QColor &color, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    // a disc of the colour, its edge a little darker
    p.setPen(QPen(color.darker(150), 1));
    p.setBrush(color);
    p.drawEllipse(QRectF(0.5, 0.5, size - 1, size - 1));
    return QIcon(pixmap);
}

// a ring that shows on any colour under it: a white ring inside a black one
void paintMarker(QPainter &p, const QPointF &at)
{
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(Qt::black, 3));
    p.drawEllipse(at, 6, 6);
    p.setPen(QPen(Qt::white, 1.5));
    p.drawEllipse(at, 6, 6);
}
}

/**
 * The theme with the colour as its accent
 */
class AccentPreview : public QWidget
{
public:
    using QWidget::QWidget;

    void setPreview(const QPalette &palette)
    {
        m_palette = palette;
        update();
    }

    QSize sizeHint() const override
    {
        return QSize(132, 84);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QSizeF size(132, 84);
        ThemePreview::paint(p, QRectF(QPointF((width() - size.width()) / 2, (height() - size.height()) / 2), size), m_palette);
    }

private: /* data */
    QPalette m_palette;
};

/**
 * Hue, left to right; arrow keys move it by a degree, Page Up and Down by ten
 */
class HueStrip : public QWidget
{
public:
    HueStrip(std::function<void(int)> changed, QWidget *parent) : QWidget(parent), m_changed(changed)
    {
        setFocusPolicy(Qt::StrongFocus);
        setMinimumSize(200, 18);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setCursor(Qt::PointingHandCursor);
    }

    int hue() const
    {
        return m_hue;
    }
    void setHue(int hue)
    {
        m_hue = qBound(0, hue, 359);
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        // the bar: every hue, left to right
        const QRectF bar = barRect();
        QLinearGradient gradient(bar.topLeft(), bar.topRight());
        for(int i = 0; i <= 6; i++)
        {
            gradient.setColorAt(i / 6.0, QColor::fromHsv((i * 60) % 360, 255, 255));
        }
        p.setPen(Qt::NoPen);
        p.setBrush(gradient);
        p.drawRoundedRect(bar, bar.height() / 2, bar.height() / 2);
        // the marker at the current hue
        paintMarker(p, QPointF(bar.left() + bar.width() * m_hue / 359.0, bar.center().y()));
        // the focus frame around the whole strip
        if(hasFocus())
        {
            p.setPen(QPen(palette().color(QPalette::Highlight), 1, Qt::DotLine));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);
        }
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        pick(event->pos().x());
    }
    void mouseMoveEvent(QMouseEvent *event) override
    {
        pick(event->pos().x());
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        int step = 0;
        switch(event->key())
        {
        case Qt::Key_Left:
            step = -1;
            break;
        case Qt::Key_Right:
            step = 1;
            break;
        case Qt::Key_PageDown:
            step = -10;
            break;
        case Qt::Key_PageUp:
            step = 10;
            break;
        default:
            QWidget::keyPressEvent(event);
            return;
        }
        setHue(m_hue + step);
        m_changed(m_hue);
    }

private:
    QRectF barRect() const
    {
        return QRectF(8, 3, width() - 16, height() - 6);
    }
    void pick(int x)
    {
        const QRectF bar = barRect();
        setHue(qRound((x - bar.left()) / bar.width() * 359));
        m_changed(m_hue);
    }

private: /* data */
    std::function<void(int)> m_changed;
    int m_hue = 0;
};

/**
 * Saturation left to right, brightness bottom to top, for one hue; arrow keys move by one
 * percent
 */
class SaturationValueField : public QWidget
{
public:
    SaturationValueField(std::function<void(qreal, qreal)> changed, QWidget *parent) : QWidget(parent), m_changed(changed)
    {
        setFocusPolicy(Qt::StrongFocus);
        setMinimumSize(200, 120);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setCursor(Qt::CrossCursor);
    }

    void setHue(int hue)
    {
        m_hue = hue;
        update();
    }
    void setSaturationValue(qreal saturation, qreal value)
    {
        m_saturation = qBound<qreal>(0.0, saturation, 1.0);
        m_value = qBound<qreal>(0.0, value, 1.0);
        update();
    }
    qreal saturation() const
    {
        return m_saturation;
    }
    qreal value() const
    {
        return m_value;
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        // the field: white to the full hue across, then darkened toward black downwards
        const QRectF field = fieldRect();
        QPainterPath shape;
        shape.addRoundedRect(field, 4, 4);
        p.setClipPath(shape);
        QLinearGradient saturation(field.topLeft(), field.topRight());
        saturation.setColorAt(0, Qt::white);
        saturation.setColorAt(1, QColor::fromHsv(m_hue, 255, 255));
        p.fillRect(field, saturation);
        QLinearGradient value(field.topLeft(), field.bottomLeft());
        value.setColorAt(0, QColor(0, 0, 0, 0));
        value.setColorAt(1, Qt::black);
        p.fillRect(field, value);
        p.setClipping(false);
        // the marker at the current saturation and brightness
        paintMarker(p, QPointF(field.left() + field.width() * m_saturation, field.bottom() - field.height() * m_value));
        // the focus frame around the whole field
        if(hasFocus())
        {
            p.setPen(QPen(palette().color(QPalette::Highlight), 1, Qt::DotLine));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);
        }
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        pick(event->pos());
    }
    void mouseMoveEvent(QMouseEvent *event) override
    {
        pick(event->pos());
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        qreal s = m_saturation, v = m_value;
        switch(event->key())
        {
        case Qt::Key_Left:
            s -= 0.01;
            break;
        case Qt::Key_Right:
            s += 0.01;
            break;
        case Qt::Key_Down:
            v -= 0.01;
            break;
        case Qt::Key_Up:
            v += 0.01;
            break;
        default:
            QWidget::keyPressEvent(event);
            return;
        }
        setSaturationValue(s, v);
        m_changed(m_saturation, m_value);
    }

private:
    QRectF fieldRect() const
    {
        return QRectF(QPointF(6, 6), QSizeF(width() - 12, height() - 12));
    }
    void pick(const QPoint &at)
    {
        const QRectF field = fieldRect();
        setSaturationValue((at.x() - field.left()) / field.width(), (field.bottom() - at.y()) / field.height());
        m_changed(m_saturation, m_value);
    }

private: /* data */
    std::function<void(qreal, qreal)> m_changed;
    int m_hue = 0;
    qreal m_saturation = 1.0;
    qreal m_value = 1.0;
};

AccentColorDialog::AccentColorDialog(const QColor &initial, const QColor &systemAccent,
                                     std::function<QPalette(const QColor &)> preview, QWidget *parent)
    : QDialog(parent), m_preview(preview)
{
    setWindowTitle(tr("Accent color"));
    // the system's accent first
    QList<QPair<QColor, QString>> suggestions;
    if(systemAccent.isValid())
    {
        suggestions.append({systemAccent, tr("The system's accent color")});
    }
    for(auto name: SUGGESTED)
    {
        suggestions.append({QColor(name), QString(name)});
    }
    build(initial, suggestions);
}

AccentColorDialog::AccentColorDialog(const QColor &initial, const QList<QColor> &suggestions,
                                     std::function<QPalette(const QColor &)> preview, const QString &title, QWidget *parent)
    : QDialog(parent), m_preview(preview)
{
    setWindowTitle(title);
    QList<QPair<QColor, QString>> named;
    for(auto &color : suggestions)
    {
        named.append({color, color.name()});
    }
    build(initial, named);
}

void AccentColorDialog::build(const QColor &initial, const QList<QPair<QColor, QString>> &suggestions)
{
    auto layout = new QVBoxLayout(this);

    m_previewWidget = new AccentPreview(this);
    layout->addWidget(m_previewWidget);

    // the suggestions in a row
    auto swatches = new QHBoxLayout();
    swatches->addStretch(1);
    auto addSwatch = [this, swatches](const QColor &color, const QString &tip) {
        auto button = new QToolButton(this);
        button->setAutoRaise(true);
        button->setIconSize(QSize(22, 22));
        button->setIcon(swatch(color, 22));
        button->setToolTip(tip);
        button->setAccessibleName(tip);
        connect(button, &QToolButton::clicked, this, [this, color]() { setColor(color); });
        swatches->addWidget(button);
    };
    for(auto &suggestion : suggestions)
    {
        addSwatch(suggestion.first, suggestion.second);
    }
    swatches->addStretch(1);
    layout->addLayout(swatches);

    m_field = new SaturationValueField([this](qreal saturation, qreal value) {
        m_color = QColor::fromHsvF(m_hue->hue() / 359.0, saturation, value);
        colorChanged(true);
    }, this);
    layout->addWidget(m_field, 1);
    m_hue = new HueStrip([this](int hue) {
        m_field->setHue(hue);
        m_color = QColor::fromHsvF(hue / 359.0, m_field->saturation(), m_field->value());
        colorChanged(true);
    }, this);
    layout->addWidget(m_hue);

    auto hexRow = new QHBoxLayout();
    auto hexLabel = new QLabel(tr("&Hex"), this);
    m_hex = new QLineEdit(this);
    m_hex->setValidator(new QRegExpValidator(QRegExp("#?[0-9a-fA-F]{0,6}"), m_hex));
    m_hex->setMaxLength(7);
    hexLabel->setBuddy(m_hex);
    hexRow->addStretch(1);
    hexRow->addWidget(hexLabel);
    hexRow->addWidget(m_hex);
    hexRow->addStretch(1);
    layout->addLayout(hexRow);
    connect(m_hex, &QLineEdit::textEdited, this, [this](const QString &text) {
        QString hex = text.startsWith('#') ? text : "#" + text;
        if(hex.size() != 7)
        {
            return;
        }
        QColor color(hex);
        if(color.isValid())
        {
            m_color = color;
            m_hue->setHue(qMax(0, color.hsvHue()));
            m_field->setHue(m_hue->hue());
            m_field->setSaturationValue(color.hsvSaturationF(), color.valueF());
            colorChanged(false);
        }
    });

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    setColor(initial.isValid() ? initial : QColor(SUGGESTED[0]));
}

QColor AccentColorDialog::color() const
{
    return m_color;
}

void AccentColorDialog::setColor(const QColor &color)
{
    m_color = color.toRgb();
    // greys have no hue: keep the strip where it was
    if(color.hsvHue() >= 0)
    {
        m_hue->setHue(color.hsvHue());
    }
    m_field->setHue(m_hue->hue());
    m_field->setSaturationValue(color.hsvSaturationF(), color.valueF());
    colorChanged(true);
}

void AccentColorDialog::colorChanged(bool updateHex)
{
    if(updateHex)
    {
        m_hex->setText(m_color.name());
    }
    m_previewWidget->setPreview(m_preview(m_color));
}
