#include "WebDownloadDialog.h"

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineDownloadItem>

#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QAction>
#include <QProgressBar>
#include <QDebug>

#include "tasks/Task.h"
#include "widgets/ProgressWidget.h"

class WebEngineDownloadTask : public Task
{
public:
	explicit WebEngineDownloadTask(QWebEngineDownloadItem *dl, QObject *parent = nullptr)
		: Task(parent), m_dl(dl)
	{
		connect(m_dl, &QWebEngineDownloadItem::downloadProgress, this, &WebEngineDownloadTask::setProgress);
		connect(m_dl, &QWebEngineDownloadItem::stateChanged, this, &WebEngineDownloadTask::stateChanged);
	}

	bool canAbort() const override { return true; }
	bool abort() override
	{
		m_dl->cancel();
		return true;
	}
	void executeTask() override
	{
		setStatus(tr("Downloading %1...").arg(m_dl->url().toString()));
	}

	void stateChanged(const QWebEngineDownloadItem::DownloadState state)
	{
		switch (state) {
		case QWebEngineDownloadItem::DownloadCancelled:
			emitFailed(tr("The download was cancelled by the user"));
			break;
		case QWebEngineDownloadItem::DownloadInterrupted:
			emitFailed(tr("The download was interrupted"));
			break;
		case QWebEngineDownloadItem::DownloadCompleted:
			emitSucceeded();
			break;
		default: break;
		}
	}

	QWebEngineDownloadItem *m_dl;
};

WebDownloadDialog::WebDownloadDialog(const QUrl &url, const QDir &dir, QWidget *parent)
	: QDialog(parent), m_dir(dir)
{
	m_webview = new QWebEngineView(this);

	auto createButtonFromAction = [](QAction *action, QWidget *parent)
	{
		QPushButton *btn = new QPushButton(parent);
		btn->setIcon(action->icon());
		btn->setToolTip(action->toolTip());
		btn->setWhatsThis(action->whatsThis());
		btn->setCheckable(action->isCheckable());
		btn->setChecked(action->isChecked());
		WebDownloadDialog::connect(btn, &QPushButton::clicked, action, &QAction::trigger);
		WebDownloadDialog::connect(action, &QAction::triggered, btn, &QPushButton::setChecked);
		return btn;
	};

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(m_backBtn = createButtonFromAction(m_webview->pageAction(QWebEnginePage::Back), this), 0, 0);
	layout->addWidget(m_forwardBtn = createButtonFromAction(m_webview->pageAction(QWebEnginePage::Forward), this), 0, 1);
	layout->addWidget(m_reloadBtn = createButtonFromAction(m_webview->pageAction(QWebEnginePage::Reload), this), 0, 2);

	QLineEdit *urlEdit = new QLineEdit(this);
	urlEdit->setReadOnly(true);
	urlEdit->setEnabled(false);
	urlEdit->setText(m_webview->url().toString());
	connect(m_webview, &QWebEngineView::urlChanged, urlEdit, [urlEdit](const QUrl &url) { urlEdit->setText(url.toString()); });
	layout->addWidget(urlEdit, 0, 3);

	QProgressBar *progressBar = new QProgressBar(this);
	progressBar->setMinimum(0);
	progressBar->setMaximum(100);
	connect(m_webview, &QWebEngineView::loadStarted, progressBar, &QProgressBar::show);
	connect(m_webview, &QWebEngineView::loadFinished, progressBar, &QProgressBar::hide);
	connect(m_webview, &QWebEngineView::loadProgress, progressBar, &QProgressBar::setValue);
	layout->addWidget(progressBar, 0, 4);

	layout->addWidget(m_webview, 1, 0, 1, 5);

	m_progress = new ProgressWidget(this);
	m_progress->hide();
	layout->addWidget(m_progress, 2, 0, 1, 5);

	connect(m_webview->page()->profile(), &QWebEngineProfile::downloadRequested, this, &WebDownloadDialog::downloadRequested);

	m_webview->load(url);
}

void WebDownloadDialog::runNonModal(const QUrl &url, const QDir &dir, QWidget *parent)
{
	WebDownloadDialog dlg(url, dir, parent);
	connect(&dlg, &WebDownloadDialog::completed, &dlg, &WebDownloadDialog::accept);
	dlg.exec();
}

void WebDownloadDialog::downloadRequested(QWebEngineDownloadItem *dl)
{
	m_backBtn->setEnabled(false);
	m_forwardBtn->setEnabled(false);
	m_reloadBtn->setEnabled(false);
	m_webview->setEnabled(false);
	m_progress->show();

	dl->setPath(m_dir.absoluteFilePath(QFileInfo(dl->url().path()).fileName()));
	qDebug() << "Caught download for" << dl->url().toString() << ", it will be downloaded to" << dl->path();

	m_task = std::make_shared<WebEngineDownloadTask>(dl, this);
	emit starting(m_task);
	connect(m_task.get(), &Task::finished, this, &WebDownloadDialog::completed);
	m_progress->start(m_task);

	dl->accept();
}
