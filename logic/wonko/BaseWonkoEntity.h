#pragma once

#include <QObject>
#include <memory>

#include "multimc_logic_export.h"

class Task;

class MULTIMC_LOGIC_EXPORT BaseWonkoEntity
{
public:
	virtual ~BaseWonkoEntity();

	using Ptr = std::shared_ptr<BaseWonkoEntity>;

	virtual Task *remoteUpdateTask() = 0;
	virtual Task *localUpdateTask() = 0;
	virtual void merge(const std::shared_ptr<BaseWonkoEntity> &other) = 0;

	void store() const;
	virtual QString localFilename() const = 0;
	virtual QJsonObject serialized() const = 0;

	bool isComplete() const { return m_localLoaded || m_remoteLoaded; }

	bool isLocalLoaded() const { return m_localLoaded; }
	bool isRemoteLoaded() const { return m_remoteLoaded; }

	void notifyLocalLoadComplete();
	void notifyRemoteLoadComplete();

private:
	bool m_localLoaded = false;
	bool m_remoteLoaded = false;
};
