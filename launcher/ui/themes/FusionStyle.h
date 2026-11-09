#pragma once

#include <QProxyStyle>

/**
 * Fusion, with the platform's own message box icons (information, warning, critical,
 * question): Fusion's are old, low resolution pixmaps (GH-1791). They come from the platform's
 * style: Windows' and macOS' system icons, Breeze's on KDE. Where that style is Fusion too
 * (most Linux desktops), Fusion's.
 * And a toolbar extension arrow in the palette's colours: Fusion's is black (GH-5665).
 */
class FusionStyle : public QProxyStyle
{
public:
    explicit FusionStyle(const QString &platformStyle);
    virtual ~FusionStyle();

    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = nullptr,
                       const QWidget *widget = nullptr) const override;

private: /* data */
    QStyle *m_platform = nullptr;
};
