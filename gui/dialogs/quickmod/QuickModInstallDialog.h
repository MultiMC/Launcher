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

	void setInitialMods(const QList<QuickMod *> mods);

private
slots:
	void downloadNextMod();

	void urlCaught(QNetworkReply *reply);
	void processReply(QNetworkReply *reply, QuickModVersionPtr version);
	void downloadProgress(const qint64 current, const qint64 max);
	void downloadCompleted();

	bool checkForIsDone();

private:
	Ui::QuickModInstallDialog *ui;

	BaseInstance *m_instance;

	QList<QuickMod *> m_initialMods;
	QList<QuickModVersionPtr> m_modVersions;
	QuickModVersionPtr m_currentVersion;
	QList<QUrl> m_downloadingUrls;

	void install(const QuickModVersionPtr version);
};
