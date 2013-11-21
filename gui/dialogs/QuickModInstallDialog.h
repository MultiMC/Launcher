#pragma once

#include <QWidget>
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

class QuickModInstallDialog : public QWidget
{
	Q_OBJECT

public:
	explicit QuickModInstallDialog(BaseInstance *instance, QWidget *parent = 0);
	~QuickModInstallDialog();

public
slots:
	void addMod(QuickMod* mod, bool isInitial = false);

private
slots:
	void urlCaught(QNetworkReply *reply);
	void downloadProgress(const qint64 current, const qint64 max);

private:
	Ui::QuickModInstallDialog *ui;
	QStackedLayout *m_stack;

	BaseInstance *m_instance;

	QMap<WebDownloadNavigator *, QPair<QuickMod *, int> > m_webModMapping;
};
