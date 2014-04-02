#pragma once

#include <QAbstractListModel>
#include "logic/BaseInstance.h"
#include "logic/tasks/Task.h"

#include "Screenshot.h"

class ScreenshotList : public QAbstractListModel
{
	Q_OBJECT
public:
	ScreenshotList(InstancePtr instance, QObject *parent = 0);

	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;

	int rowCount(const QModelIndex &parent) const;

	Qt::ItemFlags flags(const QModelIndex &index) const;

	Task *load();

	void loadShots(QList<ScreenshotPtr> shots)
	{
		m_screenshots = shots;
	}

	QList<ScreenshotPtr> screenshots() const
	{
		return m_screenshots;
	}

	InstancePtr instance() const
	{
		return m_instance;
	}

	void deleteSelected(class ScreenshotDialog *dialog);

signals:

public
slots:

private:
	QList<ScreenshotPtr> m_screenshots;
	InstancePtr m_instance;
};

class ScreenshotLoadTask : public Task
{
	Q_OBJECT

public:
	explicit ScreenshotLoadTask(ScreenshotList *list);
	~ScreenshotLoadTask();

	QList<ScreenshotPtr> screenShots() const
	{
		return m_results;
	}

protected:
	virtual void executeTask();

private:
	ScreenshotList *m_list;
	QList<ScreenshotPtr> m_results;
};
