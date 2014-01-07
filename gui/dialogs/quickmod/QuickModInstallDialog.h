#pragma once

#include <QDialog>
#include <QMap>
#include <QPair>
#include <memory>

#include "logic/quickmod/QuickModVersion.h"

namespace Ui
{
class QuickModInstallDialog;
}

class QNetworkReply;
class WebDownloadNavigator;
class BaseInstance;
class QuickMod;

class QuickModInstallDialog : public QDialog
{
	Q_OBJECT

public:
	explicit QuickModInstallDialog(BaseInstance *instance, QWidget *parent = 0);
	~QuickModInstallDialog();

public
slots:
	virtual int exec();

	bool addMod(QuickMod *mod, bool isInitial = false,
				const QString &versionFilter = QString());

private
slots:
	void urlCaught(QNetworkReply *reply);
	void downloadProgress(const qint64 current, const qint64 max);
	void downloadCompleted();

	void newModRegistered(QuickMod *mod);

	void checkForIsDone();

private:
	Ui::QuickModInstallDialog *ui;

	BaseInstance *m_instance;

	QuickMod *m_initialMod;
	QMap<QuickMod *, QuickModVersionPtr> m_trackedMods;
	QList<QUrl> m_pendingDependencyUrls;
	QList<QString> m_pendingInstallations;

	QMap<WebDownloadNavigator *, QPair<QuickMod *, QuickModVersionPtr>> m_webModMapping;

	void install(QuickMod *mod, const QuickModVersionPtr version);
};
