#pragma once

#include <QDialog>
#include "DownloadProgressDialog.h"

namespace Ui
{
class ChooseInstallModDialog;
}

class QItemSelection;
class KCategorizedView;
class ModFilterProxyModel;
class BaseInstance;

class ChooseInstallModDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ChooseInstallModDialog(BaseInstance *instance, QWidget *parent = 0);
	~ChooseInstallModDialog();

public
slots:
	void resolveSingleMod(QuickMod *mod);

private
slots:
	void on_installButton_clicked();
	void on_cancelButton_clicked();
	void on_categoriesLabel_linkActivated(const QString &link);
	void on_tagsLabel_linkActivated(const QString &link);
	void on_fulltextEdit_textChanged();
	void on_tagsEdit_textChanged();
	void on_categoryBox_currentTextChanged();
	void on_addButton_clicked();
	void on_updateButton_clicked();
	void modSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

	void modLogoUpdated();

	void setupCategoryBox();

private:
	Ui::ChooseInstallModDialog *ui;

	QuickMod *m_currentMod;

	BaseInstance *m_instance;

	DownloadProgressDialog *dialog;

	KCategorizedView *m_view;
	ModFilterProxyModel *m_model;
};
