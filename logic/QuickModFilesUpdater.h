#pragma once

#include <QObject>
#include <QDir>

class QuickMod;
class QuickModsList;

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

signals:
	void clearMods();
	void addedMod(QuickMod *mod);

	void error(const QString &message);

private
slots:
	void receivedMod();
	void get(const QUrl &url);
	void readModFiles();

private:
	QuickModsList *m_list;
	QDir m_quickmodDir;

	static QString fileName(const QuickMod *mod);
};
