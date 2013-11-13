#pragma once

#include <QObject>
#include <QFileInfo>
#include <QList>

class QWizard;
class QWizardPage;

class WelcomePage;
class ChooseInstancePage;
class ChooseVersionPage;
class WebNavigationPage;
class DownloadPage;
class FinishPage;

class QuickModFile;

class QuickModHandler : public QObject
{
	Q_OBJECT
public:
	QuickModHandler(const QStringList& arguments, QObject* parent = 0);

	static bool shouldTakeOver(const QStringList& arguments);

	QString instanceId() const;

private:
	QWizard* m_wizard;
	WelcomePage* m_welcomePage;
	ChooseInstancePage* m_chooseInstancePage;
	ChooseVersionPage* m_chooseVersionPage;
	WebNavigationPage* m_webNavigationPage;
	DownloadPage* m_downloadPage;
	FinishPage* m_finishPage;

	QList<QuickModFile*>* m_files;
};
