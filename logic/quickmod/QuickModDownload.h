#pragma once

#include <QString>
#include <QJsonObject>

class QuickModDownload
{
public:
	enum DownloadType
	{
		Direct = 1,
		Parallel,
		Sequential,
		Encoded,
		Maven
	};

	QString url;
	DownloadType type;
	int priority;
	QString hint;
	QString group;

	QJsonObject toJson() const;
};