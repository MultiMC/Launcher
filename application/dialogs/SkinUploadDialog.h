#pragma once

#include <QDialog>
#include <minecraft/auth/Account.h>

namespace Ui
{
    class SkinUploadDialog;
}

class SkinUploadDialog : public QDialog {
    Q_OBJECT
public:
    explicit SkinUploadDialog(AccountPtr acct, QWidget *parent = 0);
    virtual ~SkinUploadDialog() {};

public slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

    void on_skinBrowseBtn_clicked();

protected:
    AccountPtr m_acct;

private:
    Ui::SkinUploadDialog *ui;
};
