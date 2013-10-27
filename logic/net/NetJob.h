#pragma once
#include <QtNetwork>
#include <QLabel>
#include "NetAction.h"
#include "ByteArrayDownload.h"
#include "FileDownload.h"
#include "CacheDownload.h"
#include "HttpMetaCache.h"
#include "ForgeXzDownload.h"
#include "logic/tasks/ProgressProvider.h"

class NetJob;
typedef std::shared_ptr<NetJob> NetJobPtr;

class NetJob : public ProgressProvider
{
	Q_OBJECT
public:
	explicit NetJob(QString job_name) : ProgressProvider(), m_job_name(job_name) {};

	template <typename T>
	bool addNetAction(T action)
	{
		NetActionPtr base = std::static_pointer_cast<NetAction>(action); 
		base->index_within_job = downloads.size();
		downloads.append(action);
		parts_progress.append(part_info());
		total_progress++;
		return true;
	}

	NetActionPtr operator[](int index)
	{
		return downloads[index];
	}
	;
	NetActionPtr first()
	{
		if (downloads.size())
			return downloads[0];
		return NetActionPtr();
	}
	int size() const
	{
		return downloads.size();
	}
	virtual void getProgress(qint64 &current, qint64 &total)
	{
		current = current_progress;
		total = total_progress;
	}
	;
	virtual QString getStatus() const
	{
		return m_job_name;
	}
	;
	virtual bool isRunning() const
	{
		return m_running;
	}
	;
	QStringList getFailedFiles();
signals:
	void started();
	void progress(qint64 current, qint64 total);
	void filesProgress(int, int, int);
	void succeeded();
	void failed();
public
slots:
	virtual void start();
private
slots:
	void partProgress(int index, qint64 bytesReceived, qint64 bytesTotal);
	void partSucceeded(int index);
	void partFailed(int index);

private:
	struct part_info
	{
		qint64 current_progress = 0;
		qint64 total_progress = 1;
		int failures = 0;
	};
	QString m_job_name;
	QList<NetActionPtr> downloads;
	QList<part_info> parts_progress;
	qint64 current_progress = 0;
	qint64 total_progress = 0;
	int num_succeeded = 0;
	int num_failed = 0;
	bool m_running = false;
};
