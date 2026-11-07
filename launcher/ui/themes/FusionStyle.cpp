#include "FusionStyle.h"

#include <QStyleFactory>

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
    default:
        break;
    }
    return QProxyStyle::standardIcon(standardIcon, option, widget);
}
