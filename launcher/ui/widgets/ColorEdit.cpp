#include "ColorEdit.h"

#include <algorithm>

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPainter>
#include <QStyle>
#include <QToolButton>

/**
 * A square of a colour, as tall as the field next to it
 */
class ColorSwatch : public QWidget
{
public:
    using QWidget::QWidget;

    void setColor(const QColor &color)
    {
        m_color = color;
        update();
    }

    QSize sizeHint() const override
    {
        return QSize(m_side, m_side);
    }

    void setSide(int side)
    {
        m_side = side;
        setFixedSize(side, side);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF square = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        // the colour, its edge a little darker so a colour like the window's still shows;
        // without one, only an edge in the text's colour, faint
        if (m_color.isValid())
        {
            p.setPen(QPen(m_color.darker(150), 1));
            p.setBrush(m_color);
        }
        else
        {
            QColor edge = palette().color(QPalette::WindowText);
            edge.setAlphaF(0.4);
            p.setPen(QPen(edge, 1));
            p.setBrush(Qt::NoBrush);
        }
        p.drawRoundedRect(square, 3, 3);
    }

private: /* data */
    QColor m_color;
    int m_side = 22;
};

namespace {
// What's typed as a colour: #rrggbb, rrggbb, or a colour's name
QColor parse(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (QColor::isValidColor(trimmed))
        return QColor(trimmed);
    if (QColor::isValidColor("#" + trimmed))
        return QColor("#" + trimmed);
    return QColor();
}
}

ColorEdit::ColorEdit(QWidget *parent) : QWidget(parent)
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    // the swatch, the code, the picker
    m_swatch = new ColorSwatch(this);
    m_edit = new QLineEdit(this);
    // room for #rrggbb, no more: the colours line up in a column
    m_edit->setFixedWidth(m_edit->fontMetrics().width("#DDDDDDD") + 2 * m_edit->style()->pixelMetric(QStyle::PM_DefaultFrameWidth) + 12);
    m_pick = new QToolButton(this);
    m_pick->setText("...");
    m_pick->setToolTip(tr("Pick a color"));
    m_swatch->setSide(m_edit->sizeHint().height());
    layout->addWidget(m_swatch);
    layout->addWidget(m_edit);
    layout->addWidget(m_pick);
    layout->addStretch(1);
    setFocusProxy(m_edit);

    connect(m_pick, &QToolButton::clicked, this, &ColorEdit::pickRequested);
    connect(m_edit, &QLineEdit::textEdited, this, [this](const QString &text) {
        const QColor color = parse(text);
        m_swatch->setColor(color.isValid() ? color : (m_color.isValid() ? m_color : m_shown));
        if (color.isValid())
            emit colorTried(color);
    });
    connect(m_edit, &QLineEdit::editingFinished, this, &ColorEdit::finishEditing);
    // Enter for the picker, up and down to the colours above and below
    m_edit->installEventFilter(this);
}

bool ColorEdit::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_edit && event->type() == QEvent::KeyPress)
    {
        auto key = static_cast<QKeyEvent *>(event);
        if (key->modifiers() & ~Qt::KeypadModifier)
            return QWidget::eventFilter(watched, event);
        switch (key->key())
        {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            // what's typed first, then the picker from it; taken here, so a dialog's default
            // button doesn't get it
            finishEditing();
            emit pickRequested();
            return true;
        case Qt::Key_Up:
            moveTo(-1);
            return true;
        case Qt::Key_Down:
            moveTo(1);
            return true;
        default:
            break;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ColorEdit::moveTo(int step)
{
    // the colours next to this one: the ones beside it in the same widget, top to bottom
    QWidget *parent = parentWidget();
    if (!parent)
        return;
    auto edits = parent->findChildren<ColorEdit *>(QString(), Qt::FindDirectChildrenOnly);
    std::sort(edits.begin(), edits.end(), [](ColorEdit *a, ColorEdit *b) { return a->y() < b->y(); });
    const int next = edits.indexOf(this) + step;
    if (next < 0 || next >= edits.size())
        return;
    // leaving sets what's typed here (editingFinished); the next one's code ready to type over
    edits[next]->m_edit->setFocus(Qt::OtherFocusReason);
    edits[next]->m_edit->selectAll();
}

void ColorEdit::finishEditing()
{
    if (!m_edit->isModified())
        return;
    m_edit->setModified(false);
    const QColor color = parse(m_edit->text());
    if (color.isValid() && color != m_color)
    {
        emit colorEdited(color);
        return;
    }
    // nothing to set: back to what it was
    setColor(m_color, m_shown);
    emit editingAbandoned();
}

void ColorEdit::setColor(const QColor &color, const QColor &shown)
{
    m_color = color;
    m_shown = shown;
    m_edit->setText(color.isValid() ? color.name() : QString());
    m_edit->setModified(false);
    showSwatch();
}

void ColorEdit::showSwatch()
{
    m_swatch->setColor(m_color.isValid() ? m_color : m_shown);
}

bool ColorEdit::abandonEditing()
{
    if (!m_edit->isModified())
        return false;
    setColor(m_color, m_shown);
    emit editingAbandoned();
    return true;
}

QColor ColorEdit::color() const
{
    return m_color;
}

void ColorEdit::setPlaceholderText(const QString &text)
{
    m_edit->setPlaceholderText(text);
}

QLineEdit *ColorEdit::lineEdit() const
{
    return m_edit;
}
