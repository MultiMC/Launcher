#pragma once

#include <QDialog>
#include <QMap>
#include <QPair>
#include <memory>

#include "logic/quickmod/QuickModVersion.h"

#include "logic/BaseInstance.h"

namespace Ui
{
class QuickModInstallDialog;
}

class QNetworkReply;
class WebDownloadNavigator;
class BaseInstance;
class QuickMod;
class QTreeWidgetItem;

class QuickModInstallDialog : public QDialog
{
	Q_OBJECT

public:
	explicit QuickModInstallDialog(InstancePtr instance, QWidget *parent = 0);
	~QuickModInstallDialog();

public
slots:
	virtual int exec();

	void setInitialMods(const QList<QuickMod *> mods);
	QList<QuickModVersionPtr> modVersions() const
	{
		return m_resolvedVersions;
	}

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

	InstancePtr m_instance;

	bool install(QuickModVersionPtr version, QTreeWidgetItem *item = 0);

	QList<QuickMod *> m_initialMods;
	QList<QuickModVersionPtr> m_modVersions;
	QList<QuickModVersionPtr> m_resolvedVersions;
	QList<QUrl> m_downloadingUrls;
};
