#include "QuickModHandler.h"

#include <QStringList>
#include <QDir>

#include <QApplication>
#include <QBoxLayout>
#include <QStackedLayout>
#include <QWebView>
#include <QRadioButton>
#include <QProgressBar>
#include <QLabel>
#include <QWizard>
#include <QMessageBox>

#include <QHeaderView>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QListWidget>
#include <QListWidgetItem>

#include <QNetworkReply>
#include <QStandardPaths>
#include <QDebug>

#include <iostream>

#include "MultiMC.h"
#include "logic/BaseInstance.h"
#include "logic/lists/InstanceList.h"
#include "logic/lists/IconList.h"
#include "depends/quazip/JlCompress.h"

#include "QuickModFile.h"
#include "IconDownloader.h"

struct WorkingObject
{
	WorkingObject() : version(0) {}
	QuickModVersion* version;
	QList<QNetworkReply*> replies;
};

class WebDownloadCatcher : public QWidget
{
	Q_OBJECT
public:
	WebDownloadCatcher(QWidget* parent = 0) : QWidget(parent), m_layout(new QVBoxLayout), m_view(new QWebView(this)), m_bar(new QProgressBar(this))
	{
		m_view->setHtml(tr("<h1>Loading...</h1>"));

		connect(m_view, SIGNAL(loadProgress(int)), m_bar, SLOT(setValue(int)));
		connect(m_view->page(), SIGNAL(unsupportedContent(QNetworkReply*)), this, SIGNAL(caughtUrl(QNetworkReply*)));
		m_view->page()->setForwardUnsupportedContent(true);

		m_layout->addWidget(m_view, 1);
		m_layout->addWidget(m_bar);
		setLayout(m_layout);
	}

	int workingObjectIndex;

public slots:
	void load(const QUrl& url) { m_view->load(url); }

signals:
	void caughtUrl(QNetworkReply* url);

private:
	QVBoxLayout* m_layout;
	QWebView* m_view;
	QProgressBar* m_bar;
};

class WelcomePage : public QWizardPage
{
	Q_OBJECT
	Q_PROPERTY(WelcomePage* welcomePage READ welcomePage NOTIFY welcomePageChanged)
public:
	WelcomePage(QWidget* parent = 0) :
		QWizardPage(parent), m_isError(false), m_label(new QLabel(this)), m_view(new QListWidget(this)), m_layout(new QVBoxLayout)
	{
		setTitle(tr("Welcome"));

		m_layout->addWidget(m_label);
		m_layout->addWidget(m_view);
		setLayout(m_layout);

		m_view->setVisible(false);

		registerField("welcomePage", this, "welcomePage", SIGNAL(welcomePageChanged()));
	}

	void initializePage()
	{
		foreach (QuickModFile* file, *files) {
			if (!file->open()) {
				m_isError = true;
				m_label->setText(tr("Error opening %1: %2").arg(file->fileInfo().absoluteFilePath(), file->errorString()));
				emit completeChanged();
				return;
			}
		}

		foreach (QuickModFile* file, *files) {
			QListWidgetItem* item = new QListWidgetItem(m_view);
			item->setText(file->name());
			new ListWidgetIconDownloader(item, file->icon(), this);
		}
		m_label->setText(tr("The following stuff will be installed:"));
		m_view->setVisible(true);
	}
	void cleanupPage()
	{

	}
	bool validatePage()
	{
		return true;
	}
	bool isComplete() const
	{
		return !m_isError;
	}

	WelcomePage* welcomePage() { return this; }

	QList<QuickModFile*>* files;

	// accessor methods to the working objects
	void addWorkingObject(const WorkingObject& object) { m_workingObjects.append(object); }
	WorkingObject& workingObject(const int index) { return m_workingObjects[index]; }
	int numWorkingObjects() const { return m_workingObjects.size(); }
	void clearWorkingObjects() { m_workingObjects.clear(); }

signals:
	void welcomePageChanged();

private:
	bool m_isError;
	QLabel* m_label;
	QListWidget* m_view;
	QVBoxLayout* m_layout;

	QList<WorkingObject> m_workingObjects;
};
class ChooseInstancePage : public QWizardPage
{
	Q_OBJECT
	Q_PROPERTY(QString mcVersion READ mcVersion NOTIFY mcVersionChanged)
	Q_PROPERTY(QString instanceId READ instanceId NOTIFY instanceChanged)
public:
	ChooseInstancePage(QWidget* parent = 0) :
		QWizardPage(parent), m_widget(new QTableWidget(this)), m_isInited(false)
	{
		setTitle(tr("Choose Instance"));

		QVBoxLayout* layout = new QVBoxLayout;
		layout->addWidget(new QLabel(tr("Choose the instance to install stuff into"), this));
		layout->addWidget(m_widget);
		setLayout(layout);

		m_widget->setColumnCount(2);
		m_widget->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("MC Version"));
		m_widget->setSelectionMode(QTableWidget::SingleSelection);
		m_widget->setSelectionBehavior(QTableWidget::SelectRows);
		m_widget->verticalHeader()->setVisible(false);

		connect(m_widget->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
				this, SIGNAL(completeChanged()));
		connect(m_widget->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
				this, SIGNAL(mcVersionChanged()));
		connect(m_widget->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
				this, SIGNAL(instanceChanged()));
		connect(m_widget, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(instanceClicked(int,int)));

		registerField("mcVersion", this, "mcVersion", SIGNAL(mcVersionChanged));
		registerField("instanceId", this, "instanceId", SIGNAL(instanceChanged()));
	}

	void initializePage()
	{
		if (!m_isInited) {
			m_widget->setRowCount(MMC->instances()->count());
			for (int i = 0; i < MMC->instances()->count(); ++i) {
				InstancePtr instance = MMC->instances()->at(i);
				QTableWidgetItem* name = new QTableWidgetItem(instance->name());
				name->setIcon(MMC->icons()->getIcon(instance->iconKey()));
				name->setData(Qt::UserRole, instance->id());
				name->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
				QTableWidgetItem* mcVersion = new QTableWidgetItem(instance->currentVersionId());
				mcVersion->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
				m_widget->setItem(i, 0, name);
				m_widget->setItem(i, 1, mcVersion);
			}
			m_widget->selectRow(0);
		}
	}
	void cleanupPage()
	{

	}
	bool validatePage()
	{
		return true;
	}
	bool isComplete() const { return !m_widget->selectedItems().isEmpty(); }

	QString instanceId() const
	{
		if (m_widget->selectionModel()->hasSelection()) {
			return m_widget->item(m_widget->currentRow(), 0)->data(Qt::UserRole).toString();
		} else {
			return QString();
		}
	}

	QList<QuickModFile*>* files;

	QString mcVersion() const {
		if (instanceId().isNull()) {
			return QString();
		}
		return MMC->instances()->getInstanceById(instanceId())->currentVersionId();
	}

signals:
	void mcVersionChanged();
	void instanceChanged();

private slots:
	void instanceClicked(const int row, const int column)
	{
		m_widget->selectRow(row);
		wizard()->next();
	}

private:
	// QUESTION change to treeview (groups)?
	QTableWidget* m_widget;
	bool m_isInited;
};
class ChooseVersionPage : public QWizardPage
{
	Q_OBJECT
public:
	ChooseVersionPage(QWidget* parent = 0) :
		QWizardPage(parent), files(0), m_treeView(new QTreeWidget(this))
	{
		setCommitPage(true);
		setButtonText(QWizard::CommitButton, tr("Download"));
		setTitle(tr("Choose Version"));

		QVBoxLayout* layout = new QVBoxLayout;
		setLayout(layout);
		layout->addWidget(new QLabel(tr("Please choose the files to install:")));
		layout->addWidget(m_treeView);

		connect(m_treeView, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(itemClicked(QTreeWidgetItem*,int)));
		connect(m_treeView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(itemDoubleClicked(QTreeWidgetItem*,int)));

		m_treeView->setColumnCount(4);
		m_treeView->setHeaderLabels(QStringList() << tr("Name") << tr("Version") << tr("MC Versions") << tr("Type"));
		m_treeView->setSelectionMode(QTreeWidget::MultiSelection);
		m_treeView->setSelectionBehavior(QTreeWidget::SelectRows);
	}

	void initializePage()
	{
		m_treeView->clear();
		foreach (QuickModFile* file, *files) {
			QTreeWidgetItem* item = new QTreeWidgetItem(m_treeView);
			item->setText(0, file->name());
			new TreeWidgetIconDownloader(item, file->icon(), this);
			bool hasEnabledItem = false;
			foreach (QuickModVersion* version, file->versions()) {
				QTreeWidgetItem* vItem = new QTreeWidgetItem(item);
				vItem->setText(1, version->name());
				vItem->setText(2, version->mcVersions().join(", "));
				vItem->setText(3, version->modTypeString());
				vItem->setSelected(!hasEnabledItem);
				if (version->mcVersions().contains(field("mcVersion").toString())) {
					vItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
					hasEnabledItem = true;
				} else {
					vItem->setFlags(Qt::NoItemFlags);
				}
				m_versionItemMapping.insert(version, vItem);
			}
			item->setExpanded(hasEnabledItem);
			m_treeView->addTopLevelItem(item);
			m_fileItemMapping.insert(file, item);
		}
	}
	void cleanupPage()
	{

	}
	bool validatePage()
	{
		WelcomePage* page = field("welcomePage").value<WelcomePage*>();
		page->clearWorkingObjects();
		foreach (QTreeWidgetItem* item, m_treeView->selectedItems()) {
			if (m_versionItemMapping.values().contains(item)) {
				WorkingObject object;
				object.version = m_versionItemMapping.key(item);
				page->addWorkingObject(object);
			}
		}

		return true;
	}

	QList<QuickModFile*>* files;

private slots:
	// TODO this works, but is not perfect (kinda slow, also dragging may select multiple nodes)
	void itemClicked(QTreeWidgetItem* item, const int column)
	{
		// not a version, probably a file
		if (!m_versionItemMapping.values().contains(item)) {
			return;
		}

		QTreeWidgetItem* parent = item->parent();
		for (int i = 0; i < parent->childCount(); ++i) {
			parent->child(i)->setSelected(false);
		}

		item->setSelected(true);
	}
	void itemDoubleClicked(QTreeWidgetItem* item, const int column)
	{
		itemClicked(item, column);
		// only allow double clicking for next step if we only have one thing
		if (m_fileItemMapping.size() == 1) {
			wizard()->next();
		}
	}

private:
	QTreeWidget* m_treeView;
	QMap<QuickModFile*, QTreeWidgetItem*> m_fileItemMapping;
	QMap<QuickModVersion*, QTreeWidgetItem*> m_versionItemMapping;
};
class WebNavigationPage : public QWizardPage
{
	Q_OBJECT
public:
	WebNavigationPage(QWidget* parent = 0) :
		QWizardPage(parent), m_layout(new QStackedLayout(this))
	{
		setTitle(tr("Web Navigation"));
		setLayout(m_layout);
	}

	void initializePage()
	{
		WelcomePage* page = field("welcomePage").value<WelcomePage*>();
		for (int i = 0; i < page->numWorkingObjects(); ++i) {
			foreach (const QUrl& url, page->workingObject(i).version->urls()) {
				WebDownloadCatcher* view = new WebDownloadCatcher(this);
				view->load(url);
				view->workingObjectIndex = i;
				connect(view, SIGNAL(caughtUrl(QNetworkReply*)), this, SLOT(urlCaught(QNetworkReply*)));
				m_layout->addWidget(view);
				m_views.append(view);
			}
		}
		m_layout->setCurrentIndex(0);
	}
	void cleanupPage()
	{

	}
	bool validatePage()
	{
		return true;
	}
	bool isComplete() const
	{
		WelcomePage* page = field("welcomePage").value<WelcomePage*>();
		for (int i = 0; i < page->numWorkingObjects(); ++i) {
			if (page->workingObject(i).replies.size() != page->workingObject(i).version->urls().size()) {
				return false;
			}
		}

		return true;
	}

private slots:
	void urlCaught(QNetworkReply* reply)
	{
		WebDownloadCatcher* view = qobject_cast<WebDownloadCatcher*>(sender());
		field("welcomePage").value<WelcomePage*>()->workingObject(view->workingObjectIndex).replies.append(reply);
		if (isComplete()) {
			wizard()->next();
		}
		emit completeChanged();
	}

private:
	QStackedLayout* m_layout;
	QList<WebDownloadCatcher*> m_views;
};
class DownloadPage : public QWizardPage
{
	Q_OBJECT
public:
	DownloadPage(QWidget* parent = 0) :
		QWizardPage(parent), m_widget(new QTableWidget(this))
	{
		setTitle(tr("Downloading"));

		QVBoxLayout* layout = new QVBoxLayout;
		layout->addWidget(m_widget);
		setLayout(layout);

		m_widget->setColumnCount(2);
		m_widget->setHorizontalHeaderLabels(QStringList() << tr("File") << tr("Progress"));
	}

	void initializePage()
	{
		WelcomePage* page = field("welcomePage").value<WelcomePage*>();
		int row = 0;
		for (int i = 0; i < page->numWorkingObjects(); ++i) {
			foreach (QNetworkReply* reply, page->workingObject(i).replies) {
				m_widget->setRowCount(m_widget->rowCount() + 1);
				QProgressBar* bar = new QProgressBar;
				m_widget->setItem(row, 0, new QTableWidgetItem(fileName(reply->url())));
				m_widget->setCellWidget(row, 1, bar);
				m_replyToColumnMapping.insert(reply, row);
				reply->setProperty("progressbar", QVariant::fromValue(bar));
				reply->setProperty("workingObjectIndex", i);
				connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadChanged(qint64,qint64)));
				connect(reply, SIGNAL(finished()), this, SLOT(downloadFinished()));
				++row;
			}
		}
	}
	void cleanupPage()
	{

	}
	bool validatePage()
	{
		return true;
	}
	bool isComplete() const
	{
		WelcomePage* page = field("welcomePage").value<WelcomePage*>();
		for (int i = 0; i < page->numWorkingObjects(); ++i) {
			if (!page->workingObject(i).replies.isEmpty()) {
				return false;
			}
		}

		return true;
	}

private:
	static QString fileName(const QUrl& url)
	{
		const QString path = url.path();
		return path.mid(path.lastIndexOf('/')+1);
	}
	static bool saveDeviceToFile(QIODevice* device, const QString& file)
	{
		QFile out(file);
		if (!out.open(QFile::WriteOnly | QFile::Truncate)) {
			return false;
		}
		device->seek(0);
		QByteArray data = device->readAll();
		return data.size() == out.write(data);
	}
	static QDir dirEnsureExists(const QString& dir, const QString& path)
	{
		QDir dir_ = QDir::current();

		// first stage
		if (!dir_.exists(dir)) {
			dir_.mkpath(dir);
		}
		dir_.cd(dir);

		// second stage
		if (!dir_.exists(path)) {
			dir_.mkpath(path);
		}
		dir_.cd(path);

		return dir_;
	}

private slots:
	void downloadFinished()
	{
		QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
		WelcomePage* page = field("welcomePage").value<WelcomePage*>();
		QuickModVersion* version = page->workingObject(reply->property("workingObjectIndex").toInt()).version;
		InstancePtr instance = MMC->instances()->getInstanceById(field("instanceId").toString());
		QDir dir;
		bool extract = false;
		switch (version->modType()) {
		case QuickModVersion::ForgeMod:
			dir = dirEnsureExists(instance->minecraftRoot(), "mods");
			extract = false;
			break;
		case QuickModVersion::ForgeCoreMod:
			dir = dirEnsureExists(instance->minecraftRoot(), "coremods");
			extract = false;
			break;
		case QuickModVersion::ResourcePack:
			dir = dirEnsureExists(instance->minecraftRoot(), "resourcepacks");
			extract = false;
			break;
		case QuickModVersion::ConfigPack:
			dir = dirEnsureExists(instance->minecraftRoot(), "config");
			extract = true;
			break;
		}

		if (extract) {
			const QString path = reply->url().path();
			// TODO more file formats. KArchive?
			if (path.endsWith(".zip")) {
				const QString zipFileName = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).absoluteFilePath(fileName(reply->url()));
				if (saveDeviceToFile(reply, zipFileName)) {
					JlCompress::extractDir(zipFileName, dir.absolutePath());
				}
			} else {
				qWarning("Trying to extract an unknown file type %s", qPrintable(reply->url().toString()));
			}
		} else {
			QFile file(dir.absoluteFilePath(fileName(reply->url())));
			if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
				m_widget->removeCellWidget(m_replyToColumnMapping.value(reply), 1);
				m_widget->setItem(m_replyToColumnMapping.value(reply), 1, new QTableWidgetItem(tr("Error")));
				QMessageBox::critical(this, tr("Error"), tr("Error saving %1: %2").arg(file.fileName(), file.errorString()));
				return;
			}
			reply->seek(0);
			file.write(reply->readAll());
			file.close();
			reply->close();
			reply->deleteLater();
			page->workingObject(reply->property("workingObjectIndex").toInt()).replies.removeAll(reply);
		}
		emit completeChanged();
	}
	void downloadChanged(qint64 val, qint64 max)
	{
		QProgressBar* bar = qobject_cast<QProgressBar*>(sender()->property("progressbar").value<QObject*>());
		bar->setMaximum(max);
		bar->setValue(val);
	}

private:
	QTableWidget* m_widget;
	QMap<QNetworkReply*, int> m_replyToColumnMapping;
};
class FinishPage : public QWizardPage
{
	Q_OBJECT
public:
	FinishPage(QWidget* parent = 0) :
		QWizardPage(parent),
		m_layout(new QVBoxLayout),
		m_launchInstanceBox(new QRadioButton(tr("Launch Instance"), this)),
		m_launchMMCBox(new QRadioButton(tr("Launch MultiMC"), this)),
		m_quitBox(new QRadioButton(tr("Quit"), this))
	{
		setTitle(tr("Done"));

		m_layout->addWidget(new QLabel(tr("Everything has been installed. What should be done now?"), this));
		m_layout->addWidget(m_launchInstanceBox);
		m_layout->addWidget(m_launchMMCBox);
		m_layout->addWidget(m_quitBox);
		setLayout(m_layout);
	}

	void initializePage()
	{
		m_quitBox->setChecked(true);
	}
	void cleanupPage()
	{

	}
	bool validatePage()
	{
		if (m_quitBox->isChecked()) {
			qApp->quit();
		} else if (m_launchInstanceBox->isChecked()) {
			// TODO launch the instance
		} else if (m_launchMMCBox->isChecked()) {
			MMC->main_gui();
		}
		wizard()->hide();
		return true;
	}
	int nextId() { return -1; }

private:
	QVBoxLayout* m_layout;
	QRadioButton* m_launchInstanceBox;
	QRadioButton* m_launchMMCBox;
	QRadioButton* m_quitBox;
};

QuickModHandler::QuickModHandler(const QStringList& arguments, QObject *parent) :
	QObject(parent), m_wizard(new QWizard())
{
	m_files = new QList<QuickModFile*>();

	foreach (const QString& argument, arguments.mid(1))
	{
		QFileInfo file(argument);
		if (file.exists() && file.completeSuffix().toLower() == "quickmod")
		{
			QuickModFile* qmFile = new QuickModFile(this);
			qmFile->setFileInfo(file);
			m_files->push_back(qmFile);
		}
		else
		{
			std::cout << "Unknown argument for QuickModHandler: " << qPrintable(argument) << std::endl;
		}
	}

	m_wizard->addPage(m_welcomePage = new WelcomePage(m_wizard));
	m_wizard->addPage(m_chooseInstancePage = new ChooseInstancePage(m_wizard));
	m_wizard->addPage(m_chooseVersionPage = new ChooseVersionPage(m_wizard));
	m_wizard->addPage(m_webNavigationPage = new WebNavigationPage(m_wizard));
	m_wizard->addPage(m_downloadPage = new DownloadPage(m_wizard));
	m_wizard->addPage(m_finishPage = new FinishPage(m_wizard));

	m_welcomePage->files = m_files;
	m_chooseInstancePage->files = m_files;
	m_chooseVersionPage->files = m_files;

	connect(m_wizard, SIGNAL(rejected()), qApp, SLOT(quit()));

	m_wizard->showMaximized();
}

bool QuickModHandler::shouldTakeOver(const QStringList &arguments)
{
	if (arguments.size() > 1)
	{
		QFileInfo file(arguments[1]);
		if (file.exists())
		{
			return true;
		}
	}
	return false;
}

#include "QuickModHandler.moc"
