/*
 * Copyright 2022 kb1000
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QString>
#include <QMetaType>
#include <QUrl>

namespace Modrinth {
struct Modpack {
    QString id;

    QString name;
    QUrl iconUrl;
    QString author;
    QString description;

    bool metadataLoaded = false;
    QString wikiUrl;
    QString body;
};
}

Q_DECLARE_METATYPE(Modrinth::Modpack)
