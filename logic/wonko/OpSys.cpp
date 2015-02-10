/* Copyright 2013-2015 MultiMC Contributors
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

#include "OpSys.h"

#include <QString>

OpSys OpSys::fromString(const QString &string)
{
	if (string == "linux")
	{
		return Linux;
	}
	if (string == "windows")
	{
		return Windows;
	}
	if (string == "osx")
	{
		return OSX;
	}
	return Other;
}

OpSys OpSys::currentSystem()
{
	return
#if defined(Q_OS_WIN)
		Windows
#elif defined(Q_OS_MAC)
		OSX
#elif defined(Q_OS_LINUX)
		Linux
#endif
		;
}

QString OpSys::toString() const
{
	switch (m_system)
	{
	case Linux:
		return "linux";
	case OSX:
		return "osx";
	case Windows:
		return "windows";
	default:
		return "other";
	}
}
