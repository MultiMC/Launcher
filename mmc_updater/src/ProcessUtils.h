#pragma once

#include <QString>
#include <QProcess>

#include <list>

/** A set of functions to get information about the current
 * process and launch new processes.
 */
namespace ProcessUtils
{
enum Errors
{
	/** Status code returned by runElevated() if launching
	 * the elevated process fails.
	 */
	RunElevatedFailed = 255,
	/** Status code returned by runSync() if the application
	 * cannot be started.
	 */
	RunFailed = -8,
	/** Status code returned by runSync() if waiting for
	 * the application to exit and reading its status code fails.
	 */
	WaitFailed = -1
};

Q_PID currentProcessId();

/** Returns the absolute path to the main binary for
 * the current process.
 */
QString currentProcessPath();

/** Start a process and wait for it to finish before
 * returning its exit code.
 *
 * Returns -1 if the process cannot be started.
 */
int runSync(const QString &executable, const std::list<QString> &args);

/** Start a process and return without waiting for
 * it to finish.
 */
void runAsync(const QString &executable, const std::list<QString> &args);

/** Wait for a process to exit.
 * Returns true if the process was found and has exited or false
 * otherwise.
 */
bool waitForProcess(Q_PID pid);
}
