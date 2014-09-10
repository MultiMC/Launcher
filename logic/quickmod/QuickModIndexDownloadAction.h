#pragma once

#include "QuickModBaseDownloadAction.h"

class QuickModIndexList;

typedef std::shared_ptr<class NetJob> NetJobPtr;

class QuickModIndexDownloadAction : public QuickModBaseDownloadAction
{
	Q_OBJECT
public:
	explicit QuickModIndexDownloadAction(const QUrl &url, NetJob *netjob);

public:
	QString m_expectedUid;

	NetJob *m_job;

private:
	bool handle(const QByteArray &data);

	static QuickModIndexList *m_indexList;
};
