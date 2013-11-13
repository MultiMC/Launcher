#include "QuickModHandler.h"

#include <QStringList>
#include <QWizard>
#include <QDebug>
#include <QRadioButton>
#include <QBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QProgressBar>
#include <QStackedLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <iostream>

#include "MultiMC.h"
#include "logic/BaseInstance.h"
#include "logic/lists/InstanceList.h"

#include "QuickModFile.h"

class WelcomePage : public QWizardPage
{
	Q_OBJECT
public:
	WelcomePage(QWidget* parent = 0) :
		QWizardPage(parent), m_isError(false), m_label(new QLabel(this)), m_layout(new QVBoxLayout)
	{
		m_layout->addWidget(m_label);
		setLayout(m_layout);
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

	QList<QuickModFile*>* files;

private:
	bool m_isError;
	QLabel* m_label;
	QVBoxLayout* m_layout;
};
class ChooseInstancePage : public QWizardPage
{
	Q_OBJECT
	Q_PROPERTY(QString mcVersion READ mcVersion NOTIFY mcVersionChanged)
public:
	ChooseInstancePage(QWidget* parent = 0) :
		QWizardPage(parent), m_widget(new QTableWidget(this)), m_isInited(false)
	{
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

		registerField("mcVersion", this, "mcVersion", SIGNAL(mcVersionChanged));
	}

	void initializePage()
	{
		if (!m_isInited) {
			m_widget->setRowCount(MMC->instances()->count());
			for (int i = 0; i < MMC->instances()->count(); ++i) {
				InstancePtr instance = MMC->instances()->at(i);
				QTableWidgetItem* name = new QTableWidgetItem(instance->name());
				name->setData(Qt::UserRole, instance->id());
				QTableWidgetItem* mcVersion = new QTableWidgetItem(instance->intendedVersionId());
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
		return MMC->instances()->getInstanceById(instanceId())->intendedVersionId();
	}

signals:
	void mcVersionChanged();

private:
	// TODO change to treeview (groups)
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

		QVBoxLayout* layout = new QVBoxLayout;
		setLayout(layout);
		layout->addWidget(new QLabel(tr("Please choose the files to install:")));
		layout->addWidget(m_treeView);

		m_treeView->setColumnCount(4);
		m_treeView->setHeaderLabels(QStringList() << tr("Name") << tr("Version") << tr("MC Versions") << tr("Type"));
	}

	void initializePage()
	{
		m_treeView->clear();
		foreach (QuickModFile* file, *files) {
			QTreeWidgetItem* item = new QTreeWidgetItem(m_treeView);
			item->setText(0, file->name());
			bool hasEnabledItem = false;
			foreach (QuickModVersion* version, file->versions()) {
				QTreeWidgetItem* vItem = new QTreeWidgetItem(item);
				vItem->setText(1, version->name());
				vItem->setText(2, version->mcVersions().join(", "));
				vItem->setText(3, version->modTypeString());
				if (version->mcVersions().contains(field("mcVersion").toString())) {
					vItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
					hasEnabledItem = true;
				} else {
					vItem->setFlags(Qt::NoItemFlags);
				}
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
		return true;
	}

	QList<QuickModFile*>* files;

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
		QWizardPage(parent)
	{
		setCommitPage(true);
	}

	void initializePage()
	{

	}
	void cleanupPage()
	{

	}
	bool validatePage()
	{
		return true;
	}
	bool isComplete()
	{
		return true;
	}
};
class DownloadPage : public QWizardPage
{
	Q_OBJECT
public:
	DownloadPage(QWidget* parent = 0) :
		QWizardPage(parent)
	{
		setCommitPage(true);
	}

	void initializePage()
	{

	}
	void cleanupPage()
	{

	}
	bool validatePage()
	{
		return true;
	}
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
			// FIXME doesn't work
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

	m_wizard->show();
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

QString QuickModHandler::instanceId() const
{
	return m_chooseInstancePage->instanceId();
}

#include "QuickModHandler.moc"
