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
#include "classfile.h"
#include "javautils.h"

#include <QFile>
#include <QDebug>
#include <quazipfile.h>

namespace javautils
{

QString GetMinecraftJarVersion(QString jarName)
{
	QString version = MCVer_Unknown;

	// check if minecraft.jar exists
	QFile jar(jarName);
	if (!jar.exists())
		return version;

	// open minecraft.jar
	QuaZip zip(&jar);
	if (!zip.open(QuaZip::mdUnzip))
		return version;

	// open Minecraft.class
	zip.setCurrentFile("net/minecraft/client/Minecraft.class", QuaZip::csSensitive);
	QuaZipFile Minecraft(&zip);
	if (!Minecraft.open(QuaZipFile::ReadOnly))
		return version;

	// read Minecraft.class
	qint64 size = Minecraft.size();
	char *classfile = new char[size];
	Minecraft.read(classfile, size);

	// parse Minecraft.class
	try
	{
		char *temp = classfile;
		java::classfile MinecraftClass(temp, size);
		java::constant_pool constants = MinecraftClass.constants;
		for (java::constant_pool::container_type::const_iterator iter = constants.begin();
			 iter != constants.end(); iter++)
		{
			const java::constant &constant = *iter;
			if (constant.type != java::constant::j_string_data)
				continue;
			const std::string &str = constant.str_data;
			if (str.compare(0, 20, "Minecraft Minecraft ") == 0)
			{
				version = str.substr(20).data();
				break;
			}
		}
	}
	catch (java::classfile_exception &)
	{
	}

	// clean up
	delete[] classfile;
	Minecraft.close();
	zip.close();
	jar.close();

	return version;
}

OptiFineParsedVersion getOptiFineVersionInfoFromJar(const QString &jarName)
{
	OptiFineParsedVersion version;

	// check if minecraft.jar exists
	QFile jar(jarName);
	if (!jar.exists())
		return version;

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

	// read Class.class
	qint64 size = Config.size();
	char *classfile = new char[size];
	Config.read(classfile, size);

	// parse Config.class
	try
	{
		char *temp = classfile;
		java::classfile ConfigClass(temp, size);
		java::constant_pool constants = ConfigClass.constants;
		enum { MCVer, Edition, Release, None } in = None;
		for (java::constant_pool::container_type::const_iterator iter = constants.begin();
			 iter != constants.end(); iter++)
		{
			const java::constant &constant = *iter;
			if (constant.type != java::constant::j_string_data)
				continue;
			const QString str = QString::fromStdString(constant.str_data);
			if (str.startsWith('L') && str.endsWith(';'))
			{
				continue;
			}
			switch (in)
			{
			case MCVer:
				version.MC_VERSION = str;
				break;
			case Edition:
				version.OF_EDITON = str;
				break;
			case Release:
				version.OF_RELEASE = str;
				break;
			case None:
				break;
			}

			if (str == "MC_VERSION")
			{
				in = MCVer;
			}
			else if (str == "OF_EDITION")
			{
				in = Edition;
			}
			else if (str == "OF_RELEASE")
			{
				in = Release;
			}
			else
			{
				in = None;
			}
		}
	}
	catch (java::classfile_exception &)
	{
	}

	// clean up
	delete[] classfile;
	Config.close();
	zip.close();
	jar.close();

	return version;
}

}
