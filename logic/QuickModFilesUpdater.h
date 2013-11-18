#pragma once

#include <QObject>
#include <QDir>

class QuickMod;
class QuickModsList;
class Mod;

/**
 * Takes care of regulary checking for updates to quickmod files, and is also responsible for
 * keeping track of them
 *
 * All access to this class should be via signals and slots (in case we at some point want
 * threading)
 */
class QuickModFilesUpdater : public QObject
{
	Q_OBJECT
public:
	QuickModFilesUpdater(QuickModsList *list);

public
slots:
	void registerFile(const QUrl &url);
	void update();
	void ensureExists(const Mod &mod);

signals:
	void clearMods();
	void addedMod(QuickMod *mod);

	void error(const QString &message);

private
slots:
	void receivedMod(int notused);
	void get(const QUrl &url);
	void readModFiles();

private:
	QuickModsList *m_list;
	QDir m_quickmodDir;

	void saveQuickMod(const QuickMod *mod);
	bool parseQuickMod(const QString &fileName, QuickMod *mod);

	static QString fileName(const QuickMod *mod);
};
