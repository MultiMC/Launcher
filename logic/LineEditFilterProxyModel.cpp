#include "LineEditFilterProxyModel.h"

#include <QTimer>
#include <QLineEdit>

LineEditFilterProxyModel::LineEditFilterProxyModel(QLineEdit *lineedit)
	: QSortFilterProxyModel(lineedit)
{
	m_timer = new QTimer(this);
	connect(m_timer, &QTimer::timeout, this, &LineEditFilterProxyModel::timeout);
	connect(lineedit, &QLineEdit::textChanged, this, &LineEditFilterProxyModel::textChanged);
}

void LineEditFilterProxyModel::timeout()
{
	setFilterRegExp(QRegExp("*" + m_text + "*", Qt::CaseInsensitive, QRegExp::Wildcard));
	invalidateFilter();
}
void LineEditFilterProxyModel::textChanged(const QString &text)
{
	m_timer->stop();
	m_timer->start(100);
	m_text = text;
}
