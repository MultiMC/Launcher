#pragma once

#include "logic/tasks/Task.h"

#include <QStringList>
#include <QUrl>
#include <QNetworkReply>

class QuickModMavenFindTask : public Task
{
	Q_OBJECT
public:
	explicit QuickModMavenFindTask(const QList<QUrl> &repos, const QString &identifier);

	QUrl url() const
	{
		return m_currentUrl;
	}

protected:
	void executeTask();

private:
	QList<QUrl> m_repos;
	QString m_identifier;
	QUrl m_currentUrl;
	QUrl m_itemUrl;

	void checkNext();

private
slots:
	void networkFinished();
	void networkError(QNetworkReply::NetworkError code);
};
