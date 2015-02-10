#pragma once

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <memory>
#include "minecraft/Patch.h"
#include "wonko/WonkoPackageVersion.h"
#include "Exception.h"

class Package;

typedef std::shared_ptr<Package> PackagePtr;

// FIXME replace methods by those available in WonkoPackageVersion
class Package : public WonkoPackageVersion
{
public: /* methods */
	// FIXME: nuke? somehow?
	int getOrder()
	{
		return order;
	}
	// FIXME: nuke? somehow?
	void setOrder(int order)
	{
		this->order = order;
	}
	QString getPatchID()
	{
		return fileId;
	}
	QString getPatchName()
	{
		return name;
	}
	QString getPatchVersion()
	{
		return version;
	}
	// FIXME: replace with generic 'source' attribute?
	QString getPatchFilename()
	{
		return filename;
	}
	// FIXME: replace with generic 'source' attribute?
	void setPatchFilename(QString _filename)
	{
		filename = _filename;
	}
	// FIXME: decide what to do with this.
	bool isMoveable()
	{
		return true;
	}

public: /* data */
	// display-only fluff
	QString name;

private:
	/*
	 * WTF do we do with this?
	 *    some sort of hidden mixin?
	 *       OneSix-specific subclass that adds this?
	 */
	int order = -1;
	/*
	 * FIXME: replace with a generic 'source'.
	 * Sources should do reference counting and self-destruct when they reach zero.
	 */
	QString filename;

public:
	// patch metadata
	QString type;
	QString fileId;
	QString version;
	QMap<QString, QString> dependencies;
	QString m_releaseTimeString;
	QDateTime m_releaseTime;

	//FIXME: make this not minecraft
	Minecraft::Patch resources;
};
