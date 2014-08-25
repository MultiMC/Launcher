#pragma once

#include <memory>

class QUrl;
class QString;
template <typename T> class QList;
class SettingsObject;
class QuickModRef;
typedef std::shared_ptr<class QuickMod> QuickModPtr;

// TODO detach from QuickModsList
class QuickModIndexList
{
public:
	void setRepositoryIndexUrl(const QString &repository, const QUrl &url);
	QUrl repositoryIndexUrl(const QString &repository) const;
	bool haveRepositoryIndexUrl(const QString &repository) const;
	QList<QUrl> indices() const;

	bool haveUid(const QuickModRef &uid, const QString &repo) const;

protected:
	explicit QuickModIndexList();

	virtual QList<QuickModPtr> quickmods() const = 0;
	virtual SettingsObject *settings() const = 0;
};
