#pragma once

#include "tasks/Task.h"
#include <memory>

class BaseWonkoEntity;
class WonkoIndex;
class WonkoVersionList;
class WonkoVersion;

class BaseWonkoEntityLocalLoadTask : public Task
{
	Q_OBJECT
public:
	explicit BaseWonkoEntityLocalLoadTask(BaseWonkoEntity *entity, QObject *parent = nullptr);

protected:
	virtual QString filename() const = 0;
	virtual QString name() const = 0;
	virtual void parse(const QJsonObject &obj) const = 0;

	BaseWonkoEntity *entity() const { return m_entity; }

private:
	void executeTask() override;

	BaseWonkoEntity *m_entity;
};

class WonkoIndexLocalLoadTask : public BaseWonkoEntityLocalLoadTask
{
	Q_OBJECT
public:
	explicit WonkoIndexLocalLoadTask(WonkoIndex *index, QObject *parent = nullptr);

private:
	QString filename() const override;
	QString name() const override;
	void parse(const QJsonObject &obj) const override;
};
class WonkoVersionListLocalLoadTask : public BaseWonkoEntityLocalLoadTask
{
	Q_OBJECT
public:
	explicit WonkoVersionListLocalLoadTask(WonkoVersionList *list, QObject *parent = nullptr);

private:
	QString filename() const override;
	QString name() const override;
	void parse(const QJsonObject &obj) const override;

	WonkoVersionList *list() const;
};
class WonkoVersionLocalLoadTask : public BaseWonkoEntityLocalLoadTask
{
	Q_OBJECT
public:
	explicit WonkoVersionLocalLoadTask(WonkoVersion *version, QObject *parent = nullptr);

private:
	QString filename() const override;
	QString name() const override;
	void parse(const QJsonObject &obj) const override;

	WonkoVersion *version() const;
};
