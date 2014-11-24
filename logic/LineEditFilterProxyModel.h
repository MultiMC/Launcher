#pragma once

#include <QSortFilterProxyModel>

class QLineEdit;
class QTimer;

class LineEditFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	explicit LineEditFilterProxyModel(QLineEdit *lineedit);

	void setLineEdit(QLineEdit *edit);

private
slots:
	void timeout();
	void textChanged(const QString &text);

private:
	QTimer *m_timer;
	QString m_text;
};
