#pragma once

#include "QuickMod.h"
#include "logic/BaseVersionList.h"
#include "logic/BaseInstance.h"

class QuickModVersionList : public BaseVersionList
{
	Q_OBJECT
public:
	explicit QuickModVersionList(QuickModRef mod, InstancePtr instance, QObject *parent = 0);

	Task *getLoadTask();
	bool isLoaded();
	const BaseVersionPtr at(int i) const;
	int count() const;
	void sort()
	{
	}

protected slots:
	void updateListData(QList<BaseVersionPtr> versions)
	{
	}

private:
	QuickModRef m_mod;
	InstancePtr m_instance;
	QList<QuickModVersionRef> m_versions;
};
