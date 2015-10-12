#pragma once

#include <QWidget>

#include "pages/BasePage.h"

namespace Ui {
class WonkoPage;
}

class QSortFilterProxyModel;

class WonkoPage : public QWidget, public BasePage
{
	Q_OBJECT
public:
	explicit WonkoPage(QWidget *parent = 0);
	~WonkoPage();

	QString id() const override { return "wonko-global"; }
	QString displayName() const override { return tr("Wonko"); }
	QIcon icon() const override;
	void opened() override;

private slots:
	void on_refreshIndexBtn_clicked();
	void on_refreshFileBtn_clicked();
	void on_refreshVersionBtn_clicked();
	void on_fileSearchEdit_textChanged(const QString &search);
	void on_versionSearchEdit_textChanged(const QString &search);
	void updateCurrentVersionList(const QModelIndex &index);
	void versionListDataChanged(const QModelIndex &tl, const QModelIndex &br);

private:
	Ui::WonkoPage *ui;
	QSortFilterProxyModel *m_fileProxy;
	QSortFilterProxyModel *m_versionProxy;

	void updateVersion();
};
