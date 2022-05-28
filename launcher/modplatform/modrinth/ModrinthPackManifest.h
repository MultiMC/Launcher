/* Copyright 2022 kb1000, arthomnix
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QByteArray>
#include <QCryptographicHash>
#include <QString>
#include <QUrl>

namespace Modrinth {

const QStringList allowedDownloadDomains = {
    "cdn.modrinth.com",
    "github.com",
    "raw.githubusercontent.com",
    "gitlab.com"
};

struct File
{
    QString path;
    QCryptographicHash::Algorithm hashAlgorithm;
    QByteArray hash;
    // TODO: should this support multiple download URLs, like the JSON does?
    QUrl download;
};

}
