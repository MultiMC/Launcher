#include "QuickModInstallDialog.h"
#include "ui_QuickModInstallDialog.h"

#include <QStackedLayout>
#include <QTreeWidgetItem>
#include <QNetworkReply>
#include <QStyledItemDelegate>
#include <QProgressBar>

#include "gui/widgets/WebDownloadNavigator.h"
#include "gui/dialogs/ChooseQuickModVersionDialog.h"
#include "logic/lists/QuickModsList.h"

Q_DECLARE_METATYPE(QTreeWidgetItem *)

class ProgressItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	ProgressItemDelegate(QObject *parent = 0) : QStyledItemDelegate(parent)
	{
	}

	void paint(QPainter *painter, const QStyleOptionViewItem &opt, const QModelIndex &index) const
	{
		qulonglong progress = index.data(Qt::UserRole).toULongLong();
		qulonglong total = index.data(Qt::UserRole + 1).toULongLong();

		QStyleOptionProgressBar style;
		style.rect = opt.rect;
		style.minimum = 0;
		style.maximum = total;
		style.progress = progress;
		style.text = QString();
		style.textVisible = false;

		QApplication::style()->drawControl(QStyle::CE_ProgressBar, &style, painter);
	}
};

QuickModInstallDialog::QuickModInstallDialog(BaseInstance *instance, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::QuickModInstallDialog),
	m_stack(new QStackedLayout),
	m_instance(instance)
{
	ui->setupUi(this);
	setWindowFlags(Qt::Dialog);
	ui->webLayout->addLayout(m_stack);

	ui->progressList->setItemDelegateForColumn(3, new ProgressItemDelegate(this));
}

QuickModInstallDialog::~QuickModInstallDialog()
{
	delete ui;
}

void QuickModInstallDialog::addMod(QuickMod *mod, bool isInitial)
{
	ChooseQuickModVersionDialog dialog;
	dialog.setCanCancel(isInitial);
	dialog.setMod(mod, m_instance);
	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	auto navigator = new WebDownloadNavigator(this);
	connect(navigator, &WebDownloadNavigator::caughtUrl, this, &QuickModInstallDialog::urlCaught);
	navigator->load(mod->version(dialog.version()).url);
	m_webModMapping.insert(navigator, qMakePair(mod, dialog.version()));
	m_stack->addWidget(navigator);

	show();
	activateWindow();
}

void QuickModInstallDialog::urlCaught(QNetworkReply *reply)
{
	auto navigator = qobject_cast<WebDownloadNavigator*>(sender());
	m_stack->removeWidget(navigator);
	navigator->deleteLater();
	auto mod = m_webModMapping[navigator].first;
	auto version = mod->version(m_webModMapping[navigator].second);

	auto item = new QTreeWidgetItem(ui->progressList);
	item->setText(0, mod->name());
	item->setText(1, version.name);
	item->setText(2, reply->url().toString(QUrl::PrettyDecoded));
	reply->setProperty("item", QVariant::fromValue(item));

	connect(reply, &QNetworkReply::downloadProgress, this, &QuickModInstallDialog::downloadProgress);

	if (m_stack->isEmpty())
	{
		ui->tabWidget->setCurrentWidget(ui->downloads);
	}
}

void QuickModInstallDialog::downloadProgress(const qint64 current, const qint64 max)
{
	auto reply = qobject_cast<QNetworkReply *>(sender());
	reply->property("item").value<QTreeWidgetItem *>()->setData(3, Qt::UserRole, current);
	reply->property("item").value<QTreeWidgetItem *>()->setData(3, Qt::UserRole + 1, max);
}

#include "QuickModInstallDialog.moc"
