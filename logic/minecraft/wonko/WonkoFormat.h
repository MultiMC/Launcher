#pragma once

#include <memory>

#define CURRENT_WONKO_VERSION 0

using PackagePtr = std::shared_ptr<class Package>;
class QJsonDocument;
class QString;

class WonkoFormat
{
public:
	// loads a wonko file and converts it into a onesix package
	static PackagePtr fromJson(const QJsonDocument &doc, const QString &filename);
};
