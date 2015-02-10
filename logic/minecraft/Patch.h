#pragma once

#include <QSet>
#include <QString>
#include <QStringList>

#include "JarMod.h"
#include "Library.h"
#include "Assets.h"
#include "Libraries.h"

namespace Minecraft
{
class Resources;

struct Patch
{
	void applyTo(Resources *resoruces);

	// game and java command line params
	QString mainClass;
	QString appletClass;
	QString overwriteMinecraftArguments;
	QString addMinecraftArguments;
	QString removeMinecraftArguments;

	// a special resource that hides the minecraft asset resource logic
	std::shared_ptr<Assets> assets;

	// more game command line params, this time more special
	bool shouldOverwriteTweakers = false;
	QStringList overwriteTweakers;
	QStringList addTweakers;
	QStringList removeTweakers;

	std::shared_ptr<Libraries> libraries = std::make_shared<Libraries>();
	std::shared_ptr<Libraries> natives = std::make_shared<Libraries>();

	QSet<QString> traits; // tags
	QList<JarmodPtr> jarMods; // files of type... again.
};
}
