/* Copyright 2013 MultiMC Contributors
 *
 * Authors: Orochimarufan <orochimarufan.x3@gmail.com>
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
#include <QString>
#include "classparser_config.h"

#define MCVer_Unknown "Unknown"

namespace javautils
{
struct CLASSPARSER_EXPORT OptiFineParsedVersion
{
	QString MC_VERSION;
	QString OF_EDITION;
	QString OF_RELEASE;
	bool isValid() const { return !MC_VERSION.isNull() && !OF_EDITION.isNull() && !OF_RELEASE.isNull(); }
};
/**
 * @brief Get some info from the optifine jar
 */
CLASSPARSER_EXPORT OptiFineParsedVersion getOptiFineVersionInfoFromJar(const QString &jarName);
}
