#pragma once

#include <QDialog>
#include <QDir>
#include <memory>

class QWebEngineView;
class QWebEngineDownloadItem;
class QPushButton;
class Task;
class ProgressWidget;

class WebDownloadDialog : public QDialog
{
	Q_OBJECT
public:
	explicit WebDownloadDialog(const QUrl &url, const QDir &dir, QWidget *parent = nullptr);

	static void runNonModal(const QUrl &url, const QDir &dir, QWidget *parent = nullptr);

private slots:
	void downloadRequested(QWebEngineDownloadItem *dl);

signals:
	void starting(const std::shared_ptr<Task> &task);
	void completed();

private:
	QWebEngineView *m_webview;
	QPushButton *m_backBtn;
	QPushButton *m_forwardBtn;
	QPushButton *m_reloadBtn;
	ProgressWidget *m_progress;

	QDir m_dir;
	std::shared_ptr<Task> m_task;
};
