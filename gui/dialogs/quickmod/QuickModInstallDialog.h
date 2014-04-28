#pragma once

#include <QDialog>
#include <QMap>
#include <QPair>
#include <memory>

#include "logic/quickmod/QuickMod.h"
#include "logic/quickmod/QuickModVersion.h"

#include "logic/BaseInstance.h"

namespace Ui
{
class QuickModInstallDialog;
}

class QNetworkReply;
class WebDownloadNavigator;
class BaseInstance;
class OneSixInstance;
class QuickMod;
class QuickModInstaller;
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

	void setInitialMods(const QList<QuickModPtr> mods);
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


	/// Downloads dependency files.
	bool downloadDeps();

	/// Resolves dependencies.
	bool resolveDeps();

	/// Processes the version list. Installs any mods that are already downloaded.
	void processVersionList();

	/// Downloads all mods with direct download links.
	void runDirectDownloads();



	/// Downloads the given version as a webpage download.
	void runWebDownload(QuickModVersionPtr version);


	/// Adds a given entry to the progress list.
	QTreeWidgetItem* addProgressListEntry(QuickModVersionPtr version, const QString& url, const QString& progressMsg = "");


	/** Checks if all downloads are finished and returns true if they are.
	 *  Note that this function also updates the state of the "finish" button.
	 */
	bool checkIsDone();

	/// Sets whether the web view is shown or not.
	void setWebViewShown(bool shown);

private:
	Ui::QuickModInstallDialog *ui;
	QuickModInstaller *m_installer;

	InstancePtr m_instance;

	bool install(QuickModVersionPtr version, QTreeWidgetItem *item = 0);

	QList<QuickModPtr> m_initialMods;
	QList<QuickModVersionPtr> m_modVersions;
	QList<QuickModVersionPtr> m_resolvedVersions;
	QList<QUrl> m_downloadingUrls;
};
