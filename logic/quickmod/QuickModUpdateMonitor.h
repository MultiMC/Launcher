#pragma once

#include <QObject>
#include <memory>

class InstanceList;
class QuickModsList;
class BaseInstance;
class OneSixInstance;
class QuickMod;

class QuickModUpdateMonitor : public QObject
{
	Q_OBJECT
public:
	QuickModUpdateMonitor(std::shared_ptr<InstanceList> instances, std::shared_ptr<QuickModsList> quickmods, QObject *parent = 0);

private
slots:
	void instanceListRowsInserted(const QModelIndex &parent, const int start, const int end);
	void instanceListRowsRemoved(const QModelIndex &parent, const int start, const int end);
	void instanceListReset();
	void quickmodsListRowsInserted(const QModelIndex &parent, const int start, const int end);
	void quickmodsListRowsRemoved(const QModelIndex &parent, const int start, const int end);
	void quickmodsListReset();

	void quickmodUpdated();
	void instanceReloaded();

private:
	std::shared_ptr<InstanceList> m_instanceList;
	std::shared_ptr<QuickModsList> m_quickmodsList;

	void checkForInstance(std::shared_ptr<OneSixInstance> instance);
};
