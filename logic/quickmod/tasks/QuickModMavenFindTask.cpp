#include "QuickModMavenFindTask.h"

#include <QNetworkAccessManager>

#include "MultiMC.h"

QuickModMavenFindTask::QuickModMavenFindTask(const QList<QUrl> &repos, const QString &identifier)
	: Task((Bindable *)nullptr), m_repos(repos), m_identifier(identifier)
{
}

void QuickModMavenFindTask::executeTask()
{
	const QStringList split = m_identifier.split(':');
	if (split.size() < 3)
	{
		emitFailed(tr("Invalid maven identifier"));
		return;
	}
	// maven syntax: <group>/<artifact>/<artifact>-<version>.jar
	m_itemUrl = QUrl(QString("%1/%2/%2-%3.jar").arg(QString(split[0]).replace('.', '/'), split[2], split[3]));

	checkNext();
}

void QuickModMavenFindTask::checkNext()
{
	if (m_repos.isEmpty())
	{
		emitFailed(tr("Couldn't find maven item"));
		return;
	}
	const QUrl base = m_repos.takeFirst();
	m_currentUrl = base.resolved(m_itemUrl);
	QNetworkReply *reply = MMC->qnam()->head(QNetworkRequest(m_currentUrl));
	connect(reply, &QNetworkReply::finished, this, &QuickModMavenFindTask::networkFinished);
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkError(QNetworkReply::NetworkError)));
}

void QuickModMavenFindTask::networkFinished()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
	if (reply->error() == QNetworkReply::NoError)
	{
		emitSucceeded();
	}
}

void QuickModMavenFindTask::networkError(QNetworkReply::NetworkError code)
{
	checkNext();
}
