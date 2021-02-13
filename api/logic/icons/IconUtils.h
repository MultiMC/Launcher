#pragma once

#include <QString>
#include "launcher_logic_export.h"

namespace IconUtils {

// Given a folder and an icon key, find 'best' of the icons with the given key in there and return its path
LAUNCHER_LOGIC_EXPORT QString findBestIconIn(const QString &folder, const QString & iconKey);

// Get icon file type filter for file browser dialogs
LAUNCHER_LOGIC_EXPORT QString getIconFilter();

}
