#pragma once

#include <QObject>
#include <QDir>
#include <memory>

#include "logic/quickmod/QuickMod.h"

class QuickModsList;
class Mod;

/**
 * Takes care of regulary checking for updates to quickmod files, and is also responsible for
 * keeping track of them
 */
class QuickModFilesUpdater : public QObject
{
	Q_OBJECT
public:
	QuickModFilesUpdater(QuickModsList *list);

	// TODO use some sort of lookup
	QuickModPtr ensureExists(const Mod &mod);

	void registerFile(const QUrl &url);
	void unregisterMod(const QuickModPtr mod);

public
slots:
	void update();

signals:
	void error(const QString &message);

private
slots:
	void receivedMod(int notused);
	void failedMod(int index);
	void readModFiles();

private:
	QuickModsList *m_list;
	QDir m_quickmodDir;

	bool parseQuickMod(const QString &fileName, QuickModPtr mod);

	static QString fileName(const QuickModPtr mod);
	static QString fileName(const QString &uid);
};
