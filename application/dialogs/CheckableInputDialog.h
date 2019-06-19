#pragma once

#include <QDialog>

namespace Ui
{
    class CheckableInputDialog;
}

class CheckableInputDialog : public QDialog
{
    Q_OBJECT

public:
    CheckableInputDialog(QWidget *parent);
    ~CheckableInputDialog();

    void setText(QString text);
    void setExtraText(QString text);
    void setCheckboxText(QString checkboxText);

    bool checkboxChecked();
    QString getInput();

private:
    Ui::CheckableInputDialog *ui;

};
