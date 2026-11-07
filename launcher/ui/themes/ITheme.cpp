#include "ITheme.h"
#include "Colors.h"
#include <QStyleFactory>
#include <QPixmapCache>
#include <QDir>
#include "Application.h"

void ITheme::apply(bool)
{
    // Fusion (Qt 5.6) caches painted buttons, combo boxes, spin boxes... in QPixmapCache by
    // state and size, not palette: without this, they keep the old theme's colours. Newer Qt
    // has the palette in the key; drop this after checking the version we upgrade to.
    QPixmapCache::clear();
    QApplication::setStyle(QStyleFactory::create(qtTheme()));
    if(hasColorScheme())
    {
        QApplication::setPalette(colorScheme());
    }
    if(hasStyleSheet())
    {
        APPLICATION->setStyleSheet(appStyleSheet());
    }
    else
    {
        APPLICATION->setStyleSheet(QString());
    }
    QDir::setSearchPaths("theme", searchPaths());
    TitleBarWatcher::setStyle(titleBarStyle());
}

QPalette ITheme::fadeInactive(QPalette in, qreal bias, QColor color)
{
    auto blend = [&in, bias, color](QPalette::ColorRole role)
    {
        QColor from = in.color(QPalette::Active, role);
        QColor blended = Colors::mix(from, color, bias);
        in.setColor(QPalette::Disabled, role, blended);
    };
    blend(QPalette::Window);
    blend(QPalette::WindowText);
    blend(QPalette::Base);
    blend(QPalette::AlternateBase);
    blend(QPalette::ToolTipBase);
    blend(QPalette::ToolTipText);
    blend(QPalette::Text);
    blend(QPalette::Button);
    blend(QPalette::ButtonText);
    blend(QPalette::BrightText);
    blend(QPalette::Link);
    blend(QPalette::Highlight);
    blend(QPalette::HighlightedText);
    return in;
}
