#include <QJsonArray>
#include <QJsonDocument>
#include <modutils.h>

#include <QDebug>

#include "minecraft/MinecraftPatch.h"
#include "minecraft/RawLibrary.h"
#include "minecraft/MinecraftProfile.h"
#include "minecraft/JarMod.h"
#include "ParseUtils.h"

#include "VersionBuildError.h"

int findLibraryByName(QList<RawLibraryPtr> haystack, const GradleSpecifier &needle)
{
	int retval = -1;
	for (int i = 0; i < haystack.size(); ++i)
	{
		if (haystack.at(i)->rawName().matchName(needle))
		{
			// only one is allowed.
			if (retval != -1)
				return -1;
			retval = i;
		}
	}
	return retval;
}

void MinecraftPatch::applyTo(MinecraftResources *version)
{
	if (!mainClass.isNull())
	{
		version->mainClass = mainClass;
	}
	if (!appletClass.isNull())
	{
		version->appletClass = appletClass;
	}
	if (!assets.isNull())
	{
		version->assets = assets;
	}
	if (!overwriteMinecraftArguments.isNull())
	{
		version->minecraftArguments = overwriteMinecraftArguments;
	}
	if (!addMinecraftArguments.isNull())
	{
		version->minecraftArguments += addMinecraftArguments;
	}
	if (!removeMinecraftArguments.isNull())
	{
		version->minecraftArguments.remove(removeMinecraftArguments);
	}
	if (shouldOverwriteTweakers)
	{
		version->tweakers = overwriteTweakers;
	}
	for (auto tweaker : addTweakers)
	{
		version->tweakers += tweaker;
	}
	for (auto tweaker : removeTweakers)
	{
		version->tweakers.removeAll(tweaker);
	}
	version->jarMods.append(jarMods);
	version->traits.unite(traits);
	if (shouldOverwriteLibs)
	{
		version->libraries = overwriteLibs;
	}
	for (auto addedLibrary : addLibs)
	{
		switch (addedLibrary->insertType)
		{
		case RawLibrary::Apply:
		{
			// qDebug() << "Applying lib " << lib->name;
			int index = findLibraryByName(version->libraries, addedLibrary->rawName());
			if (index >= 0)
			{
				auto existingLibrary = version->libraries[index];
				if (!addedLibrary->m_base_url.isNull())
				{
					existingLibrary->setBaseUrl(addedLibrary->m_base_url);
				}
				if (!addedLibrary->m_hint.isNull())
				{
					existingLibrary->setHint(addedLibrary->m_hint);
				}
				if (!addedLibrary->m_absolute_url.isNull())
				{
					existingLibrary->setAbsoluteUrl(addedLibrary->m_absolute_url);
				}
				if (addedLibrary->applyExcludes)
				{
					existingLibrary->extract_excludes = addedLibrary->extract_excludes;
				}
				if (addedLibrary->isNative())
				{
					existingLibrary->m_native_classifiers = addedLibrary->m_native_classifiers;
				}
				if (addedLibrary->applyRules)
				{
					existingLibrary->setRules(addedLibrary->m_rules);
				}
			}
			else
			{
				qWarning() << "Couldn't find" << addedLibrary->rawName() << "(skipping)";
			}
			break;
		}
		case RawLibrary::Append:
		case RawLibrary::Prepend:
		{
			// find the library by name.
			const int index = findLibraryByName(version->libraries, addedLibrary->rawName());
			// library not found? just add it.
			if (index < 0)
			{
				if (addedLibrary->insertType == RawLibrary::Append)
				{
					version->libraries.append(addedLibrary);
				}
				else
				{
					version->libraries.prepend(addedLibrary);
				}
				break;
			}

			// otherwise apply differences, if allowed
			auto existingLibrary = version->libraries.at(index);
			const Util::Version addedVersion = addedLibrary->version();
			const Util::Version existingVersion = existingLibrary->version();
			// if the existing version is a hard dependency we can either use it or
			// fail, but we can't change it
			if (existingLibrary->dependType == RawLibrary::Hard)
			{
				// we need a higher version, or we're hard to and the versions aren't
				// equal
				if (addedVersion > existingVersion ||
					(addedLibrary->dependType == RawLibrary::Hard && addedVersion != existingVersion))
				{
					throw VersionBuildError(QObject::tr(
						"Error resolving library dependencies between %1 and %2.")
												.arg(existingLibrary->rawName(),
													 addedLibrary->rawName()));
				}
				else
				{
					// the library is already existing, so we don't have to do anything
				}
			}
			else if (existingLibrary->dependType == RawLibrary::Soft)
			{
				// if we are higher it means we should update
				if (addedVersion > existingVersion)
				{
					version->libraries.replace(index, addedLibrary);
				}
				else
				{
					// our version is smaller than the existing version, but we require
					// it: fail
					if (addedLibrary->dependType == RawLibrary::Hard)
					{
						throw VersionBuildError(QObject::tr(
							"Error resolving library dependencies between %1 and %2.")
													.arg(existingLibrary->rawName(),
														 addedLibrary->rawName()));
					}
				}
			}
			break;
		}
		case RawLibrary::Replace:
		{
			GradleSpecifier toReplace;
			if (addedLibrary->insertData.isEmpty())
			{
				toReplace = addedLibrary->rawName();
			}
			else
			{
				toReplace = addedLibrary->insertData;
			}
			// qDebug() << "Replacing lib " << toReplace << " with " << lib->name;
			int index = findLibraryByName(version->libraries, toReplace);
			if (index >= 0)
			{
				version->libraries.replace(index, addedLibrary);
			}
			else
			{
				qWarning() << "Couldn't find" << toReplace << "(skipping)";
			}
			break;
		}
		}
	}
	for (auto lib : removeLibs)
	{
		int index = findLibraryByName(version->libraries, lib);
		if (index >= 0)
		{
			// qDebug() << "Removing lib " << lib;
			version->libraries.removeAt(index);
		}
		else
		{
			qWarning() << "Couldn't find" << lib << "(skipping)";
		}
	}
}
