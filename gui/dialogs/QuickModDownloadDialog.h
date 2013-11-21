#ifndef QUICKMODDOWNLOADDIALOG_H
#define QUICKMODDOWNLOADDIALOG_H

#include <QDialog>
#include <QMap>

namespace Ui {
class QuickModDownloadDialog;
}

class QNetworkReply;
class QuickMod;
class WebDownloadNavigator;

class QuickModDownloadDialog : public QDialog
{
	Q_OBJECT

public:
	explicit QuickModDownloadDialog(const QMap<QuickMod*, QUrl> &mods, QWidget *parent = 0);
	~QuickModDownloadDialog();

private
slots:
	void urlCaught(QNetworkReply *reply);

private:
	Ui::QuickModDownloadDialog *ui;

	QMap<WebDownloadNavigator*, QuickMod*> m_webModMapping;
};

#endif // QUICKMODDOWNLOADDIALOG_H
