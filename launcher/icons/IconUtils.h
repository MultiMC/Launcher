#pragma once

#include <QIcon>
#include <QSize>
#include <QString>

namespace IconUtils {

// Given a folder and an icon key, find 'best' of the icons with the given key in there and return its path
QString findBestIconIn(const QString &folder, const QString & iconKey);

// Get icon file type filter for file browser dialogs
QString getIconFilter();

/// The icon fit to a box (in logical pixels, drawn at ratio device pixels each): its transparent
/// margins trimmed, small pixel art enlarged sharp, anything else smoothly. Cached.
QPixmap fitted(const QIcon &icon, const QSize &box, qreal ratio, QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off);

}
