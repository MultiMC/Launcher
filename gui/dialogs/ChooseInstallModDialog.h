#pragma once

#include <QDialog>

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

private
slots:
	void on_installButton_clicked();
	void on_cancelButton_clicked();
	void on_categoriesLabel_linkActivated(const QString &link);
	void on_tagsLabel_linkActivated(const QString &link);
	void on_fulltextEdit_textChanged();
	void modSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
	Ui::ChooseInstallModDialog *ui;

	KCategorizedView *m_view;
	ModFilterProxyModel* m_model;
};
