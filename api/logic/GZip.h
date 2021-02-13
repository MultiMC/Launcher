#pragma once
#include <QByteArray>

#include "launcher_logic_export.h"

class LAUNCHER_LOGIC_EXPORT GZip
{
public:
    static bool unzip(const QByteArray &compressedBytes, QByteArray &uncompressedBytes);
    static bool zip(const QByteArray &uncompressedBytes, QByteArray &compressedBytes);
};

