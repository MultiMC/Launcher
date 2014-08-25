#include "QuickModIndexList.h"

#include <QString>
#include <QMap>
#include <QUrl>
#include <QVariant>

#include "logic/settings/SettingsObject.h"
#include "QuickModRef.h"
#include "QuickMod.h"

QuickModIndexList::QuickModIndexList() {}

void QuickModIndexList::setRepositoryIndexUrl(const QString &repository, const QUrl &url)
{
	QMap<QString, QVariant> map = settings()->get("Indices").toMap();
	map[repository] = url.toString(QUrl::FullyEncoded);
	settings()->set("Indices", map);
}
QUrl QuickModIndexList::repositoryIndexUrl(const QString &repository) const
{
	return QUrl(settings()->get("Indices").toMap()[repository].toString(), QUrl::StrictMode);
}
bool QuickModIndexList::haveRepositoryIndexUrl(const QString &repository) const
{
	return settings()->get("Indices").toMap().contains(repository);
}
QList<QUrl> QuickModIndexList::indices() const
{
	QList<QUrl> out;
	const auto map = settings()->get("Indices").toMap();
	for (const auto value : map.values())
	{
		out.append(QUrl(value.toString(), QUrl::StrictMode));
	}
	return out;
}

bool QuickModIndexList::haveUid(const QuickModRef &uid, const QString &repo) const
{
	const auto mods = quickmods();
	for (auto mod : mods)
	{
		if (mod->uid() == uid && mod->repo() == repo)
		{
			return true;
		}
	}
	return false;
}
