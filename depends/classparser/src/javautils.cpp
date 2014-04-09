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

#include "javautils.h"
#include "ClassFile.h"

#include <QFile>
#include <QDebug>
#include <quazipfile.h>

namespace javautils
{

template<typename T>
T *getFinalStaticConstant(const Java::ClassFile *file, const std::string &field)
{
	auto f = file->field(field);
	for (auto attr : f.attributes())
	{
		if (attr.name() == "ConstantValue")
		{
			return attr.resolved()->getAs<Java::ConstantValueAttribute>()->value()->getAs<T>();
		}
	}
	return 0;
}

OptiFineParsedVersion getOptiFineVersionInfoFromJar(const QString &jarName)
{
	OptiFineParsedVersion version;

	// check if minecraft.jar exists
	QFile jar(jarName);
	if (!jar.exists())
	{
		return version;
	}

	// open minecraft.jar
	QuaZip zip(&jar);
	if (!zip.open(QuaZip::mdUnzip))
	{
		return version;
	}

	// open Config.class
	zip.setCurrentFile("Config.class", QuaZip::csSensitive);
	QuaZipFile Config(&zip);
	if (!Config.open(QuaZipFile::ReadOnly))
	{
		return version;
	}

	QDataStream stream(&Config);

	// parse Config.class
	try
	{
		Java::ClassFile file(stream);
		version.MC_VERSION = QString::fromStdString(getFinalStaticConstant<Java::StringConstant>(&file, "MC_VERSION")->string());
		version.OF_EDITION = QString::fromStdString(getFinalStaticConstant<Java::StringConstant>(&file, "OF_EDITION")->string());
		version.OF_RELEASE = QString::fromStdString(getFinalStaticConstant<Java::StringConstant>(&file, "OF_RELEASE")->string());
	}
	catch (std::exception &e)
	{
	}

	// clean up
	Config.close();
	zip.close();
	jar.close();

	return version;
}

}
