#include "IconThemeSelectionWidget.h"

#include <QDir>
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QPainter>

#include <xdgicon.h>

#include "SelectionCard.h"

namespace {
// the icon themes, by their setting value
const char *const ICON_THEMES[] = {"multimc", "pe_colored", "pe_blue", "pe_dark", "pe_light", "flat", "OSX", "iOS", "custom"};
// what a card shows of each
const char *const SAMPLES[] = {"new", "viewfolder", "settings", "help", "checkupdate", "news"};
const int ICON_SIZE = 24;
const int COLUMNS = 5;
}

/**
 * One icon theme: a few of its icons on a field
 */
class IconCard : public SelectionCard
{
public:
    IconCard(const QString &id, std::function<void()> clicked, QWidget *parent)
        : SelectionCard(id, QSize(3 * ICON_SIZE + 4 * 8, 2 * ICON_SIZE + 3 * 8), clicked, parent)
    {
        // the icon loader has one theme at a time: switch to this one for a moment
        const QString current = XdgIcon::themeName();
        XdgIcon::setThemeName(id);
        for(auto name: SAMPLES)
        {
            m_icons.append(XdgIcon::fromTheme(name).pixmap(ICON_SIZE, ICON_SIZE));
        }
        XdgIcon::setThemeName(current);
    }

protected:
    void paintPicture(QPainter &p, const QRectF &r) override
    {
        p.setPen(QPen(palette().color(QPalette::Mid), 1));
        p.setBrush(palette().color(QPalette::Base));
        p.drawRoundedRect(r.adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);
        for(int i = 0; i < m_icons.size(); i++)
        {
            QPointF at(r.left() + 8 + (i % 3) * (ICON_SIZE + 8), r.top() + 8 + (i / 3) * (ICON_SIZE + 8));
            p.drawPixmap(QRectF(at, QSizeF(ICON_SIZE, ICON_SIZE)), m_icons[i], QRectF(m_icons[i].rect()));
        }
    }

private: /* data */
    QList<QPixmap> m_icons;
};

IconThemeSelectionWidget::IconThemeSelectionWidget(QWidget *parent) : QWidget(parent)
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto grid = new QGridLayout();
    layout->addStretch(1);
    layout->addLayout(grid);
    layout->addStretch(1);
    for(auto id: ICON_THEMES)
    {
        // made by hand: only where someone did
        if(QString(id) == "custom" && !QDir("iconthemes/custom").exists())
        {
            continue;
        }
        QString themeId(id);
        auto card = new IconCard(themeId, [this, themeId]() { select(themeId); }, this);
        grid->addWidget(card, m_cards.size() / COLUMNS, m_cards.size() % COLUMNS);
        m_cards.append(card);
    }
    retranslate();
}

QString IconThemeSelectionWidget::iconTheme() const
{
    return m_iconTheme;
}

void IconThemeSelectionWidget::setIconTheme(const QString &id)
{
    m_iconTheme = id;
    for(auto card: m_cards)
    {
        card->setSelected(card->id() == id);
    }
}

void IconThemeSelectionWidget::select(const QString &id)
{
    setIconTheme(id);
    emit iconThemeChanged(id);
}

void IconThemeSelectionWidget::retranslate()
{
    for(auto card: m_cards)
    {
        const QString id = card->id();
        if(id == "multimc")
            card->setName(tr("Default"));
        else if(id == "pe_colored")
            card->setName(tr("Colored"));
        else if(id == "pe_blue")
            card->setName(tr("Blue"));
        else if(id == "pe_dark")
            card->setName(tr("Dark icons"));
        else if(id == "pe_light")
            card->setName(tr("Light icons"));
        else if(id == "flat")
            card->setName("Flat");
        else if(id == "OSX")
            card->setName("OSX");
        else if(id == "iOS")
            card->setName("iOS");
        else
            card->setName(tr("Custom"));
    }
}

void IconThemeSelectionWidget::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    QWidget::changeEvent(event);
}
