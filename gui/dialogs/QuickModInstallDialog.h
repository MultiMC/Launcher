#pragma once

#include <QDialog>
#include <QMap>
#include <QPair>

namespace Ui {
class QuickModInstallDialog;
}

class QNetworkReply;
class QStackedLayout;
class QuickMod;
class WebDownloadNavigator;
class BaseInstance;

class QuickModInstallDialog : public QDialog
{
	Q_OBJECT

public:
	explicit QuickModInstallDialog(BaseInstance *instance, QWidget *parent = 0);
	~QuickModInstallDialog();

public
slots:
	void addMod(QuickMod* mod, bool isInitial = false, const QString &versionFilter = QString());

private
slots:
	void urlCaught(QNetworkReply *reply);
	void downloadProgress(const qint64 current, const qint64 max);
	void downloadCompleted();

	void newModRegistered(QuickMod *mod);

	void checkForIsDone();

private:
	Ui::QuickModInstallDialog *ui;
	QStackedLayout *m_stack;

	BaseInstance *m_instance;

	QMap<QuickMod *, int> m_trackedMods;

	QMap<WebDownloadNavigator *, QPair<QuickMod *, int> > m_webModMapping;

	void install(QuickMod *mod, const int versionIndex);
};
