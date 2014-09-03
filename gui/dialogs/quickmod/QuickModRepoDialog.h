#pragma once

#include <QDialog>

namespace Ui
{
class QuickModRepoDialog;
}

class QuickModIndexList;

class QuickModRepoDialog : public QDialog
{
	Q_OBJECT

public:
	explicit QuickModRepoDialog(QWidget *parent = 0);
	~QuickModRepoDialog();

private slots:
	void on_removeBtn_clicked();

private:
	Ui::QuickModRepoDialog *ui;

	QuickModIndexList *m_list;
};
