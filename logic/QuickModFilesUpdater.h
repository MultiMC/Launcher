#pragma once

#include <QObject>
#include <QDir>

class QuickMod;
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

	QuickMod *ensureExists(const Mod &mod);

	void registerFile(const QUrl &url);

public
slots:
	void update();

signals:
	void error(const QString &message);

private
slots:
	void receivedMod(int notused);
	void get(const QUrl &url);
	void readModFiles();

private:
	QuickModsList *m_list;
	QDir m_quickmodDir;

	void saveQuickMod(QuickMod *mod);
	bool parseQuickMod(const QString &fileName, QuickMod *mod);

	static QString fileName(const QuickMod *mod);
};
