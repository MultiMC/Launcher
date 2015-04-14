// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include <QIcon>
#include <QPointer>
#include <QMap>

class QAction;
class QLabel;
class QAbstractButton;
class QAbstractItemModel;
class QPersistentModelIndex;

class IconRegistry
{
public:
	explicit IconRegistry();

	QStringList keys(const QString &base) const;

	void setTheme(const QString &theme);
	QString theme() const;

	void setForTarget(QObject *target, const char *property, const QString &key, const int size = -1);
	void setForTarget(QLabel *label, const QString &key, const int size = -1);
	void setForTarget(QAction *action, const QString &key, const int size = -1);
	void setForTarget(QAbstractButton *button, const QString &key, const int size = -1);
	void setForModel(QAbstractItemModel *model, const int column);

	// special function for IconProxyModel
	QIcon icon(const QString &key, const QModelIndex &index) const;

private:
	friend class ModelIconFetcher;
	friend class TargetIconFetcher;

	QIcon icon(const QString &key) const;
	QPixmap pixmap(const QString &key) const;

	QString m_theme;

	struct Target
	{
		QPointer<QObject> object;
		const char *property;
		int size;

		bool operator<(const Target &other) const
		{
			return object.data() < other.object.data();
		}
		bool operator==(const Target &other) const
		{
			return object.data() == other.object.data();
		}
	};
	QMap<Target, QString> m_targets;
	void updateTarget(const Target &target);

	QList<QPair<QAbstractItemModel *, int>> m_targetModels;

	mutable QMap<QPersistentModelIndex, QObject *> m_modelFetchers;
	mutable QMap<Target, QObject *> m_targetFetchers;

	/**
	 * Attempts to find a files for a given key, and returns all files (resolutions) for that key
	 *
	 * Search order:
	 * 1. :/<theme>/<key>/<*>
	 * 2. :/icons/<key>/<*>
	 * 3. :/key.png
	 */
	QStringList findFiles(const QString &key) const;
};
