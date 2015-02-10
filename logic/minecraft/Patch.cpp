#include <QJsonArray>
#include <QJsonDocument>
#include <modutils.h>

#include <QDebug>

#include "minecraft/Patch.h"
#include "minecraft/Library.h"
#include "minecraft/MinecraftProfile.h"
#include "minecraft/JarMod.h"

namespace Minecraft
{
void Patch::applyTo(Minecraft::Resources *version)
{
	if (!mainClass.isNull())
	{
		version->mainClass = mainClass;
	}
	if (!appletClass.isNull())
	{
		version->appletClass = appletClass;
	}
	if (assets)
	{
		assets->applyTo(version->assets);
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
	libraries->applyTo(version->libraries);
	natives->applyTo(version->natives);
}
}
