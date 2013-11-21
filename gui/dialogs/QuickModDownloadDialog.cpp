#include "QuickModDownloadDialog.h"
#include "ui_QuickModDownloadDialog.h"

#include <QStackedLayout>
#include <QTreeWidgetItem>
#include <QNetworkReply>

#include "gui/widgets/WebDownloadNavigator.h"
#include "logic/lists/QuickModsList.h"

QuickModDownloadDialog::QuickModDownloadDialog(const QMap<QuickMod *, QUrl> &mods, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::QuickModDownloadDialog)
{
	ui->setupUi(this);

	QStackedLayout *stack = new QStackedLayout(this);
	ui->webLayout->addLayout(stack);
	for (auto iterator = mods.begin(); iterator != mods.end(); iterator++)
	{
		WebDownloadNavigator *navigator = new WebDownloadNavigator(this);
		connect(navigator, &WebDownloadNavigator::caughtUrl, this, &QuickModDownloadDialog::urlCaught);
		navigator->load(iterator.value());
		stack->addWidget(navigator);
	}
}

QuickModDownloadDialog::~QuickModDownloadDialog()
{
	delete ui;
}

void QuickModDownloadDialog::urlCaught(QNetworkReply *reply)
{
	auto mod = m_webModMapping.value(qobject_cast<WebDownloadNavigator*>(sender()));
}
