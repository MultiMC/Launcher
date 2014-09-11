#pragma once

#include <QObject>
#include <QHash>
#include <memory>

class QuickModsList;
class QuickModRef;
class QTimer;

typedef std::shared_ptr<class QuickModMetadata> QuickModMetadataPtr;
typedef std::shared_ptr<class QuickModVersion> QuickModVersionPtr;

class QuickModDatabase : public QObject
{
	Q_OBJECT
public:
	QuickModDatabase(QuickModsList *list);

	void add(QuickModMetadataPtr metadata);
	void add(QuickModVersionPtr version);

	QHash<QuickModRef, QList<QuickModMetadataPtr> > metadata() const;

signals:
	void aboutToReset();
	void reset();

private:
	QuickModsList *m_list;
	//    uid            repo     data
	QHash<QString, QHash<QString, QuickModMetadataPtr>> m_metadata;
	//    uid            version  data
	QHash<QString, QHash<QString, QuickModVersionPtr>> m_versions;

	bool m_isDirty = false;
	QTimer *m_timer;

	static QString m_filename;

private slots:
	void delayedFlushToDisk();
	void flushToDisk();
	void syncFromDisk();
};
