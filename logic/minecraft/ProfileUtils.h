#pragma once
#include "Library.h"
#include "Package.h"

namespace ProfileUtils
{
typedef QStringList PatchOrder;

/// Read and parse a OneSix format order file
bool readOverrideOrders(QString path, PatchOrder &order);

/// Write a OneSix format order file
bool writeOverrideOrders(QString path, const PatchOrder &order);


/// Parse a version file in JSON format
PackagePtr parseJsonFile(const QFileInfo &fileInfo, const bool requireOrder);

/// Remove LWJGL from a patch file. This is applied to all Mojang-like profile files.
void removeLwjglFromPatch(PackagePtr patch);

}
