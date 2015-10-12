#pragma once

#include "tasks/Task.h"
#include <memory>

class BaseWonkoEntity;
class WonkoIndex;
class WonkoVersionList;
class WonkoVersion;

class BaseWonkoEntityRemoteLoadTask : public Task
{
	Q_OBJECT
public:
	explicit BaseWonkoEntityRemoteLoadTask(BaseWonkoEntity *entity, QObject *parent = nullptr);

protected:
	virtual QUrl url() const = 0;
	virtual QString name() const = 0;
	virtual void parse(const QJsonObject &obj) const = 0;

	BaseWonkoEntity *entity() const { return m_entity; }

private slots:
	void networkFinished();

private:
	void executeTask() override;

	BaseWonkoEntity *m_entity;
	std::shared_ptr<class CacheDownload> m_dl;
};

class WonkoIndexRemoteLoadTask : public BaseWonkoEntityRemoteLoadTask
{
	Q_OBJECT
public:
	explicit WonkoIndexRemoteLoadTask(WonkoIndex *index, QObject *parent = nullptr);

private:
	QUrl url() const override;
	QString name() const override;
	void parse(const QJsonObject &obj) const override;
};
class WonkoVersionListRemoteLoadTask : public BaseWonkoEntityRemoteLoadTask
{
	Q_OBJECT
public:
	explicit WonkoVersionListRemoteLoadTask(WonkoVersionList *list, QObject *parent = nullptr);

private:
	QUrl url() const override;
	QString name() const override;
	void parse(const QJsonObject &obj) const override;

	WonkoVersionList *list() const;
};
class WonkoVersionRemoteLoadTask : public BaseWonkoEntityRemoteLoadTask
{
	Q_OBJECT
public:
	explicit WonkoVersionRemoteLoadTask(WonkoVersion *version, QObject *parent = nullptr);

private:
	QUrl url() const override;
	QString name() const override;
	void parse(const QJsonObject &obj) const override;

	WonkoVersion *version() const;
};
