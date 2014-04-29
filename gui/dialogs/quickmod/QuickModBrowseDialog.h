#pragma once

#include <QDialog>

#include "logic/BaseInstance.h"
#include "logic/quickmod/QuickMod.h"

class OneSixInstance;

namespace Ui
{
class QuickModBrowseDialog;
}

class QItemSelection;
class QListView;
class ModFilterProxyModel;
class CheckboxProxyModel;

class QuickModBrowseDialog : public QDialog
{
	Q_OBJECT

public:
	explicit QuickModBrowseDialog(InstancePtr instance, QWidget *parent = 0);
	~QuickModBrowseDialog();

private
slots:
	void on_installButton_clicked();
	void on_categoriesLabel_linkActivated(const QString &link);
	void on_tagsLabel_linkActivated(const QString &link);
	void on_mcVersionsLabel_linkActivated(const QString &link);
	void on_fulltextEdit_textChanged();
	void on_tagsEdit_textChanged();
	void on_categoryBox_currentTextChanged();
	void on_mcVersionBox_currentTextChanged();
	void on_addButton_clicked();
	void on_updateButton_clicked();
	void modSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

	void modLogoUpdated();

	void setupComboBoxes();

private:
	Ui::QuickModBrowseDialog *ui;

	QuickModPtr m_currentMod;

	InstancePtr m_instance;

	QListView *m_view;
	ModFilterProxyModel *m_filterModel;
	CheckboxProxyModel *m_checkModel;
};
