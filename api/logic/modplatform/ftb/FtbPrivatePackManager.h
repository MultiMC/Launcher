#pragma once

#include <QStringList>
#include <QFile>
#include "multimc_logic_export.h"

class MULTIMC_LOGIC_EXPORT FtbPrivatePackManager {
private:
	static QStringList currentPacks;
	static QFile saveFile;

public:
	static void refresh();
	static void save();
	static QStringList getCurrentPackCodes();
	static void setCurrentPacks(QStringList newPackList);
	static void addCode(QString code);
	static void removeCode(QString code);
};
