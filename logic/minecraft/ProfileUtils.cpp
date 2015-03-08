#include "ProfileUtils.h"
#include "minecraft/VersionFilterData.h"
#include "onesix/OneSixFormat.h"
#include "Json.h"
#include <QDebug>

#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSaveFile>

#include "Json.h"

namespace ProfileUtils
{

static const int currentOrderFileVersion = 1;

bool writeOverrideOrders(QString path, const PatchOrder &order)
{
	QJsonObject obj;
	obj.insert("version", currentOrderFileVersion);
	QJsonArray orderArray;
	for(auto str: order)
	{
		orderArray.append(str);
	}
	obj.insert("order", orderArray);
	QSaveFile orderFile(path);
	if (!orderFile.open(QFile::WriteOnly))
	{
		qCritical() << "Couldn't open" << orderFile.fileName()
					 << "for writing:" << orderFile.errorString();
		return false;
	}
	auto data = QJsonDocument(obj).toJson(QJsonDocument::Indented);
	if(orderFile.write(data) != data.size())
	{
		qCritical() << "Couldn't write all the data into" << orderFile.fileName()
					 << "because:" << orderFile.errorString();
		return false;
	}
	if(!orderFile.commit())
	{
		qCritical() << "Couldn't save" << orderFile.fileName()
					 << "because:" << orderFile.errorString();
	}
	return true;
}

bool readOverrideOrders(QString path, PatchOrder &order)
{
	QFile orderFile(path);
	if (!orderFile.exists())
	{
		qWarning() << "Order file doesn't exist. Ignoring.";
		return false;
	}
	if (!orderFile.open(QFile::ReadOnly))
	{
		qCritical() << "Couldn't open" << orderFile.fileName()
					 << " for reading:" << orderFile.errorString();
		qWarning() << "Ignoring overriden order";
		return false;
	}

	// and it's valid JSON
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(orderFile.readAll(), &error);
	if (error.error != QJsonParseError::NoError)
	{
		qCritical() << "Couldn't parse" << orderFile.fileName() << ":" << error.errorString();
		qWarning() << "Ignoring overriden order";
		return false;
	}

	// and then read it and process it if all above is true.
	try
	{
		auto obj = Json::ensureObject(doc);
		// check order file version.
		auto version = Json::ensureInteger(obj, "version", Json::Required, "version");
		if (version != currentOrderFileVersion)
		{
			throw Json::JsonException(QObject::tr("Invalid order file version, expected %1")
										  .arg(currentOrderFileVersion));
		}
		auto orderArray = Json::ensureArray(obj, "order");
		for(auto item: orderArray)
		{
			order.append(Json::ensureString(item));
		}
	}
	catch (Json::JsonException &err)
	{
		qCritical() << "Couldn't parse" << orderFile.fileName() << ": bad file format";
		qWarning() << "Ignoring overriden order";
		order.clear();
		return false;
	}
	return true;
}

PackagePtr parseJsonFile(const QFileInfo &fileInfo, const bool requireOrder)
{
	QFile file(fileInfo.absoluteFilePath());
	if (!file.open(QFile::ReadOnly))
	{
		throw Json::JsonException(QObject::tr("Unable to open the version file %1: %2.")
									  .arg(fileInfo.fileName(), file.errorString()));
	}
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
	if (error.error != QJsonParseError::NoError)
	{
		throw Json::JsonException(
			QObject::tr("Unable to process the version file %1: %2 at %3.")
				.arg(fileInfo.fileName(), error.errorString())
				.arg(error.offset));
	}
	return OneSixFormat::fromJson(doc, file.fileName(), requireOrder);
}

void removeLwjglFromPatch(Minecraft::Patch &resources)
{
	const auto lwjglWhitelist =
		QSet<QString>{"net.java.jinput:jinput",	 "net.java.jinput:jinput-platform",
					  "net.java.jutils:jutils",	 "org.lwjgl.lwjgl:lwjgl",
					  "org.lwjgl.lwjgl:lwjgl_util", "org.lwjgl.lwjgl:lwjgl-platform"};
	auto filter = [&lwjglWhitelist](QList<LibraryPtr>& libs)
	{
		QList<LibraryPtr> filteredLibs;
		for (auto lib : libs)
		{
			if (!lwjglWhitelist.contains(lib->name().artifactPrefix()))
			{
				filteredLibs.append(lib);
			}
		}
		libs = filteredLibs;
	};
	filter(resources.libraries->addLibs);
	filter(resources.libraries->overwriteLibs);
	filter(resources.natives->addLibs);
	filter(resources.natives->overwriteLibs);
}
}
