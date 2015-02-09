#include "ProcessUtils.h"

#include <QProcess>
#include <QCoreApplication>

#include "FileUtils.h"
#include "Log.h"

#include <vector>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <sys/wait.h>
#endif

#ifdef Q_OS_OSX
#include <Security/Security.h>
#include <mach-o/dyld.h>
#endif

Q_PID ProcessUtils::currentProcessId()
{
	return static_cast<Q_PID>(qApp->applicationPid());
}

int ProcessUtils::runSync(const QString& executable,
						  const std::list<QString>& args)
{
	QProcess process;
	process.setProgram(executable);
	process.setArguments(QStringList::fromStdList(args));
	process.start();
	process.waitForStarted();
	process.waitForFinished();
	if (process.exitStatus() != QProcess::NormalExit)
	{
		LOG(Warn, "Child exited abnormally");
	}
	if (process.error() != QProcess::UnknownError)
	{
		LOG(Error, "Process errored: " + process.errorString());
	}
	return process.exitCode();
}

void ProcessUtils::runAsync(const QString& executable,
			  const std::list<QString>& args)
{
	QProcess::startDetached(executable, QStringList::fromStdList(args));
}

bool ProcessUtils::waitForProcess(Q_PID pid)
{
#ifdef Q_OS_UNIX
	pid_t result = ::waitpid((int)pid, 0, 0);
	if (result < 0)
	{
		LOG(Error,"waitpid() failed with error: " + std::string(strerror(errno)));
	}
	return result > 0;
#elif defined(Q_OS_WIN)
	HANDLE hProc;

	if (!(hProc = OpenProcess(SYNCHRONIZE, FALSE, pid)))
	{
		LOG(Error,"Unable to get process handle for pid " + intToStr(pid) + " last error " + intToStr(GetLastError()));
		return false;
	}

	DWORD dwRet = WaitForSingleObject(hProc, INFINITE);
	CloseHandle(hProc);

	if (dwRet == WAIT_FAILED)
	{
		LOG(Error,"WaitForSingleObject failed with error " + intToStr(GetLastError()));
	}

	return (dwRet == WAIT_OBJECT_0);
#endif
}

QString ProcessUtils::currentProcessPath()
{
#ifdef Q_OS_LINUX
	const QString path = FileUtils::canonicalPath("/proc/self/exe");
	LOG(Info,"Current process path " + path);
	return path;
#elif defined(Q_OS_OSX)
	uint32_t bufferSize = PATH_MAX;
	char buffer[bufferSize];
	_NSGetExecutablePath(buffer,&bufferSize);
	return buffer;
#else
	char fileName[MAX_PATH];
	GetModuleFileName(0 /* get path of current process */,fileName,MAX_PATH);
	return fileName;
#endif
}
