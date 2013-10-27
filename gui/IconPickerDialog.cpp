#include <MultiMC.h>
#include "IconPickerDialog.h"
#include "instancedelegate.h"
#include "ui_IconPickerDialog.h"
#include "logic/lists/IconList.h"
#include "gui/platform.h"
#include <QKeyEvent>
#include <QPushButton>
#include <QFileDialog>

IconPickerDialog::IconPickerDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::IconPickerDialog)
{
    MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
	setWindowModality(Qt::WindowModal);
	
	auto contentsWidget = ui->iconView;
	contentsWidget->setViewMode(QListView::IconMode);
	contentsWidget->setFlow(QListView::LeftToRight);
	contentsWidget->setIconSize(QSize(48, 48));
	contentsWidget->setMovement(QListView::Static);
	contentsWidget->setResizeMode(QListView::Adjust);
	contentsWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	contentsWidget->setSpacing(5);
	contentsWidget->setWordWrap(false);
	contentsWidget->setWrapping(true);
	contentsWidget->setUniformItemSizes(true);
	contentsWidget->setTextElideMode(Qt::ElideRight);
	contentsWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	contentsWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	contentsWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	contentsWidget->setItemDelegate(new ListViewDelegate());
	
	//contentsWidget->setAcceptDrops(true);
	contentsWidget->setDropIndicatorShown(true);
	contentsWidget->viewport()->setAcceptDrops(true);
	contentsWidget->setDragDropMode(QAbstractItemView::DropOnly);
	contentsWidget->setDefaultDropAction(Qt::CopyAction);
	
	contentsWidget->installEventFilter(this);
	
	contentsWidget->setModel(MMC->icons().get());
	
	auto buttonAdd = ui->buttonBox->addButton(tr("Add Icon"),QDialogButtonBox::ResetRole);
	auto buttonRemove = ui->buttonBox->addButton(tr("Remove Icon"),QDialogButtonBox::ResetRole);
	
	
	connect(buttonAdd,SIGNAL(clicked(bool)),SLOT(addNewIcon()));
	connect(buttonRemove,SIGNAL(clicked(bool)),SLOT(removeSelectedIcon()));
	
	connect
	(
		contentsWidget,
		SIGNAL(doubleClicked(QModelIndex)),
		SLOT(activated(QModelIndex))
	);
	
	connect
	(
		contentsWidget->selectionModel(),
		SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
		SLOT(selectionChanged(QItemSelection,QItemSelection))
	);
}
bool IconPickerDialog::eventFilter ( QObject* obj, QEvent* evt)
{
	if(obj != ui->iconView)
		return QDialog::eventFilter(obj ,evt);
	if (evt->type() != QEvent::KeyPress)
	{
		return QDialog::eventFilter(obj ,evt);
	}
	QKeyEvent *keyEvent = static_cast<QKeyEvent*>(evt);
	switch(keyEvent->key())
	{
		case Qt::Key_Delete:
			removeSelectedIcon();
			return true;
		case Qt::Key_Plus:
			addNewIcon();
			return true;
		default:
			break;
	}
	return QDialog::eventFilter(obj ,evt);
}

void IconPickerDialog::addNewIcon()
{
	//: The title of the select icons open file dialog
	QString selectIcons = tr("Select Icons");
	//: The type of icon files
	QStringList fileNames = QFileDialog::getOpenFileNames(this, selectIcons, QString(), tr("Icons") + "(*.png *.jpg *.jpeg)");
	MMC->icons()->installIcons(fileNames);
}

void IconPickerDialog::removeSelectedIcon()
{
	MMC->icons()->deleteIcon(selectedIconKey);
}


void IconPickerDialog::activated ( QModelIndex index )
{
	selectedIconKey = index.data(Qt::UserRole).toString();
	accept();
}


void IconPickerDialog::selectionChanged ( QItemSelection selected, QItemSelection deselected )
{
	if(selected.empty())
		return;
	
	QString key = selected.first().indexes().first().data(Qt::UserRole).toString();
	if(!key.isEmpty())
		selectedIconKey = key;
}

int IconPickerDialog::exec ( QString selection )
{
	auto list = MMC->icons();
	auto contentsWidget = ui->iconView;
	selectedIconKey = selection;
	
	
	int index_nr = list->getIconIndex(selection);
	auto model_index = list->index(index_nr);
	contentsWidget->selectionModel()->select(model_index, QItemSelectionModel::Current | QItemSelectionModel::Select);
	
	QMetaObject::invokeMethod(this, "delayed_scroll", Qt::QueuedConnection, Q_ARG(QModelIndex,model_index));
	return QDialog::exec();
}

void IconPickerDialog::delayed_scroll ( QModelIndex model_index )
{
	auto contentsWidget = ui->iconView;
	contentsWidget->scrollTo(model_index);
}


IconPickerDialog::~IconPickerDialog()
{
	delete ui;
}
