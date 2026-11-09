#include "FusionStyle.h"

#include <QApplication>
#include <QIconEngine>
#include <QPainter>
#include <QStyleFactory>
#include <QStyleOption>

#include <utility>

namespace {
/**
 * The toolbar extension arrow (») in the palette's button text colour, looked up whenever it is
 * painted: Fusion's is a black pixmap, invisible on dark toolbars (GH-5665), and the toolbar
 * asks for its icon only once, so it has to follow theme changes by itself.
 */
class ExtensionArrowEngine : public QIconEngine
{
public:
    enum class Direction
    {
        Right,
        Left,
        Down
    };

    explicit ExtensionArrowEngine(Direction direction) : m_direction(direction) {}

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State) override
    {
        // Qt's own arrow, pixel for pixel (tb_extension_arrow_h_xpm): crisp at the small sizes
        // toolbars ask for, where drawn lines blur together
        static const char *const pattern[] = {
            "XX..XX..",
            ".XX..XX.",
            "..XX..XX",
            ".XX..XX.",
            "XX..XX..",
        };
        const int columns = 8, rows = 5;
        const bool down = m_direction == Direction::Down;
        const int width = down ? rows : columns;
        const int height = down ? columns : rows;
        // whole pixels per pattern pixel: 1 up to 16 px, more on larger (HiDPI) icons
        const int unit = qMax(1, qMin(rect.width(), rect.height()) / 16);
        const QPoint origin = rect.center() - QPoint(width * unit, height * unit) / 2 + QPoint(1, 1);

        const auto group = mode == QIcon::Disabled ? QPalette::Disabled : QPalette::Active;
        const QColor color = QApplication::palette().color(group, QPalette::ButtonText);

        // the pattern's pixels; turned for down (columns become rows), mirrored for left
        for (int row = 0; row < rows; row++)
        {
            for (int column = 0; column < columns; column++)
            {
                if (pattern[row][column] != 'X')
                    continue;
                int x = m_direction == Direction::Left ? columns - 1 - column : column;
                int y = row;
                if (down)
                    std::swap(x, y);
                painter->fillRect(origin.x() + x * unit, origin.y() + y * unit, unit, unit, color);
            }
        }
    }

    // QIconEngine's own paints into a pixmap it doesn't clear first
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override
    {
        QPixmap pixmap(size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        paint(&painter, QRect(QPoint(0, 0), size), mode, state);
        return pixmap;
    }

    QIconEngine *clone() const override
    {
        return new ExtensionArrowEngine(m_direction);
    }

private: /* data */
    Direction m_direction;
};
}

FusionStyle::FusionStyle(const QString &platformStyle) : QProxyStyle(QStyleFactory::create("Fusion"))
{
    // the style's name stays Fusion's: other code tells styles apart by it
    setObjectName(baseStyle()->objectName());
    if (platformStyle.compare("fusion", Qt::CaseInsensitive) != 0)
    {
        m_platform = QStyleFactory::create(platformStyle);
    }
}

FusionStyle::~FusionStyle()
{
    delete m_platform;
}

QIcon FusionStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption *option, const QWidget *widget) const
{
    switch (standardIcon)
    {
    case SP_MessageBoxInformation:
    case SP_MessageBoxWarning:
    case SP_MessageBoxCritical:
    case SP_MessageBoxQuestion:
        if (m_platform)
        {
            QIcon icon = m_platform->standardIcon(standardIcon, option, widget);
            if (!icon.isNull())
            {
                return icon;
            }
        }
        break;
    case SP_ToolBarHorizontalExtensionButton:
    {
        const bool rtl = option ? option->direction == Qt::RightToLeft : QApplication::isRightToLeft();
        return QIcon(new ExtensionArrowEngine(rtl ? ExtensionArrowEngine::Direction::Left : ExtensionArrowEngine::Direction::Right));
    }
    case SP_ToolBarVerticalExtensionButton:
        return QIcon(new ExtensionArrowEngine(ExtensionArrowEngine::Direction::Down));
    default:
        break;
    }
    return QProxyStyle::standardIcon(standardIcon, option, widget);
}
