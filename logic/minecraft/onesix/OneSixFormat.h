#pragma once

#include <memory>

#define CURRENT_MINIMUM_LAUNCHER_VERSION 14

class QJsonDocument;
class QJsonObject;
class QString;
using PackagePtr = std::shared_ptr<class Package>;
using LibraryPtr = std::shared_ptr<class Library>;
using JarmodPtr = std::shared_ptr<class Jarmod>;

class OneSixFormat
{
public:
	static QJsonDocument toJson(PackagePtr file, bool saveOrder);
	static QJsonObject toJson(LibraryPtr raw);
	static QJsonObject toJson(JarmodPtr jarmod);
	static QJsonObject toJson(std::shared_ptr<class ImplicitRule> rule);
	static QJsonObject toJson(std::shared_ptr<class OsRule> rule);

	static PackagePtr fromJson(const QJsonDocument &doc, const QString &filename, const bool requireOrder);
	static JarmodPtr fromJson(const QJsonObject &libObj, const QString &filename);

	static LibraryPtr readRawLibraryPlus(const QJsonObject &libObj, const QString &filename);
};
