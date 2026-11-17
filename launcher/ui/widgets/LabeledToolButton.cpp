/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <QLabel>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QStyleOption>
#include "LabeledToolButton.h"
#include "icons/IconUtils.h"
#include <QApplication>
#include <QDebug>

/*
 * 
 *  Tool Button with a label on it, instead of the normal text rendering
 * 
 */

LabeledToolButton::LabeledToolButton(QWidget * parent)
    : QToolButton(parent)
    , m_label(new QLabel(this))
{
    //QToolButton::setText(" ");
    m_label->setWordWrap(true);
    m_label->setMouseTracking(false);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setTextInteractionFlags(Qt::NoTextInteraction);
    // somehow, this makes word wrap work in the QLabel. yay.
    //m_label->setMinimumWidth(100);
}

QString LabeledToolButton::text() const
{
    return m_label->text();
}

void LabeledToolButton::setText(const QString & text)
{
    m_label->setText(text);
    keepWholeContent();
}

void LabeledToolButton::setIcon(QIcon icon)
{
    m_icon = icon;
    resetIcon();
    // a picture instead of text, or another one: another height
    keepWholeContent();
}


/*!
    \reimp
*/
QSize LabeledToolButton::sizeHint() const
{
    /*
    Q_D(const QToolButton);
    if (d->sizeHint.isValid())
        return d->sizeHint;
    */
    ensurePolished();

    int w = 0, h = 0;
    QStyleOptionToolButton opt;
    initStyleOption(&opt);
    QSize sz =m_label->sizeHint();
    w = sz.width();
    h = sz.height();

    opt.rect.setSize(QSize(w, h)); // PM_MenuButtonIndicator depends on the height
    if (popupMode() == MenuButtonPopup)
        w += style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, this);
    
    QSize rawSize = style()->sizeFromContents(QStyle::CT_ToolButton, &opt, QSize(w, h), this);
    QSize sizeHint = rawSize.expandedTo(QApplication::globalStrut());
    return sizeHint;
}



void LabeledToolButton::resizeEvent(QResizeEvent * event)
{
    m_label->setGeometry(QRect(4, 4, width()-8, height()-8));
    if(!m_icon.isNull())
    {
        resetIcon();
    }
    // a new width wraps the text into more or fewer lines
    if(event->size().width() != event->oldSize().width())
    {
        keepWholeContent();
    }
    QWidget::resizeEvent(event);
}

bool LabeledToolButton::event(QEvent * event)
{
    const bool result = QToolButton::event(event);
    // after a stylesheet had its say (it polishes the button, then the style changes)
    if (event->type() == QEvent::Polish || event->type() == QEvent::StyleChange)
    {
        keepWholeContent();
    }
    return result;
}

// Never smaller than what's on it, as a minimum: a layout short of room squeezes widgets down to
// their minimums (the instance tool bar does), and the label cut the icon off, or clipped a long
// name and moved it around, with the window's height. A stylesheet's min-height (Ore UI has one)
// is a minimum too, which layouts take over the size hint. Text wraps: its height is the one at
// the button's width (the size hint's is for one line).
// Not fixed by this: Qt 5's tool bar can still place the name over the icon when it runs out of
// room (Design/Theming.md, "The instance tool bar", 2026-11-17).
void LabeledToolButton::keepWholeContent()
{
    // the label is 4 pixels in from each edge (resizeEvent), which the style's size hint
    // doesn't know about
    int height = qMax(sizeHint().height(), m_label->sizeHint().height() + 8);
    if(m_icon.isNull() && width() > 8)
    {
        height = qMax(height, m_label->heightForWidth(width() - 8) + 8);
    }
    if(height != minimumHeight())
    {
        setMinimumHeight(height);
    }
}

void LabeledToolButton::resetIcon()
{
    // fit to 80 pixels high, up to 160 wide for a wide one, as the instance view fits its icons
    // (enlarged too: QIcon alone only scales its images down)
    const QPixmap pixmap = IconUtils::fitted(m_icon, QSize(160, 80), devicePixelRatioF());
    m_label->setPixmap(pixmap);
    m_label->setMinimumHeight(80);
    m_label->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred );
}
