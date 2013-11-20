#pragma once

#include <QDialog>
#include "DownloadProgressDialog.h"

namespace Ui {
class ChooseInstallModDialog;
}

class QItemSelection;
class KCategorizedView;
class ModFilterProxyModel;

class ChooseInstallModDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ChooseInstallModDialog(QWidget *parent = 0);
	~ChooseInstallModDialog();

public slots:
	void resolveSingleMod(QuickMod* mod);

	private
	slots:
	void on_installButton_clicked();
	void on_cancelButton_clicked();
	void on_categoriesLabel_linkActivated(const QString &link);
	void on_tagsLabel_linkActivated(const QString &link);
	void on_fulltextEdit_textChanged();
	void modSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

	void on_addButton_clicked();

	void on_updateButton_clicked();

private:

	Ui::ChooseInstallModDialog *ui;

	DownloadProgressDialog *dialog;

	KCategorizedView *m_view;
	ModFilterProxyModel* m_model;
};
