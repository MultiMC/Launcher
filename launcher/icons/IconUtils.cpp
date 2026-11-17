#include "IconUtils.h"
#include <QPixmapCache>
#include <QPixmap>
#include <QImage>
#include <QIcon>

#include "FileSystem.h"
#include <QDirIterator>

#include <array>

namespace {
std::array<const char *, 6> validIconExtensions = {{
    "svg",
    "png",
    "ico",
    "gif",
    "jpg",
    "jpeg"
}};
}

namespace IconUtils{

QString findBestIconIn(const QString &folder, const QString & iconKey) {
    int best_found = validIconExtensions.size();
    QString best_filename;

    QDirIterator it(folder, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        it.next();
        auto fileInfo = it.fileInfo();

        if(fileInfo.completeBaseName() != iconKey)
            continue;

        auto extension = fileInfo.suffix();

        for(int i = 0; i < best_found; i++) {
            if(extension == validIconExtensions[i]) {
                best_found = i;
                qDebug() << i << " : " << fileInfo.fileName();
                best_filename = fileInfo.fileName();
            }
        }
    }
    return FS::PathCombine(folder, best_filename);
}

QString getIconFilter() {
    QString out;
    QTextStream stream(&out);
    stream << '(';
    for(size_t i = 0; i < validIconExtensions.size() - 1; i++) {
        if(i > 0) {
            stream << " ";
        }
        stream << "*." << validIconExtensions[i];
    }
    stream << " *." << validIconExtensions[validIconExtensions.size() - 1];
    stream << ')';
    return out;
}


// - what's in it, without its transparent margins (some modpacks' logos sit small in a wide
//   image), fit to the box, its shape kept;
// - from the biggest image the icon has (QIcon only scales its images down), or for one drawn
//   at any size (SVG), drawn at the size it's shown at;
// - a small image (32 pixels or less, as Minecraft's textures are) enlarged twice or more:
//   sharp, by the largest whole factor with nearest pixels, then smoothly for the rest, so its
//   pixels stay crisp and all the same size. Anything else smoothly: a picture enlarged with
//   nearest pixels looks crunchy, and nothing tells pixel art and pictures apart reliably
//   (Design/Theming.md, "Instance icons", 2026-11-17).
QPixmap fitted(const QIcon &icon, const QSize &box, qreal ratio, QIcon::Mode mode, QIcon::State state)
{
    if (icon.isNull())
        return QPixmap();
    const QSize pixels = box * ratio;
    const QString key = QString("fitted-icon:%1:%2x%3:%4:%5")
                            .arg(icon.cacheKey())
                            .arg(pixels.width())
                            .arg(pixels.height())
                            .arg(int(mode))
                            .arg(int(state));
    QPixmap out;
    if (QPixmapCache::find(key, &out))
        return out;

    // the biggest image it has, or one drawn at the size it's shown at
    QSize source = pixels;
    const auto sizes = icon.availableSizes(mode, state);
    if (!sizes.isEmpty())
    {
        source = sizes.first();
        for (auto &available : sizes)
        {
            if (available.width() * available.height() > source.width() * source.height())
                source = available;
        }
    }
    QImage image = icon.pixmap(source, mode, state).toImage().convertToFormat(QImage::Format_ARGB32);
    if (image.isNull())
        return QPixmap();

    // what's visible: pixels more than a tenth opaque (faint glows and shadows don't count)
    int left = image.width(), right = -1, top = image.height(), bottom = -1;
    for (int y = 0; y < image.height(); y++)
    {
        auto line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = 0; x < image.width(); x++)
        {
            if (qAlpha(line[x]) > 25)
            {
                left = qMin(left, x);
                right = qMax(right, x);
                top = qMin(top, y);
                bottom = qMax(bottom, y);
            }
        }
    }
    if (right >= left && bottom >= top)
        image = image.copy(QRect(QPoint(left, top), QPoint(right, bottom)));

    const QSize target = image.size().scaled(pixels, Qt::KeepAspectRatio);
    const int factor = target.width() / image.width();
    if (qMax(image.width(), image.height()) <= 32 && factor >= 2)
        image = image.scaled(image.size() * factor, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    if (image.size() != target)
        image = image.scaled(target, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    out = QPixmap::fromImage(image);
    out.setDevicePixelRatio(ratio);
    QPixmapCache::insert(key, out);
    return out;
}
}

