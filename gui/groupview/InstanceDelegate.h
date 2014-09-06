/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QStyledItemDelegate>
#include <QCache>

class ListViewDelegate : public QStyledItemDelegate
{
public:
	explicit ListViewDelegate(QObject *parent = 0);

	static QPixmap requestBadgePixmap(const QString &key);

protected:
	void paint(QPainter *painter, const QStyleOptionViewItem &option,
			   const QModelIndex &index) const;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
	static QCache<QString, QPixmap> m_pixmapCache;
};
