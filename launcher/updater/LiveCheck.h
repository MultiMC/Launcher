#pragma once

#include <QString>

#include <functional>

/**
 * After an update, the updater starts the new build with --alive and waits for it to report
 * in: once it runs, it writes its single-instance key into a file (Application.cpp). Shared by
 * updater v1 and v2.
 */
namespace LiveCheck
{
/// Whether a Launcher with this single-instance key is running (it answers as the server)
bool isRunning(const QString &key);

using IsRunning = std::function<bool(const QString &key)>;

/// Wait up to timeoutMs for the key to appear in file, then ask whether that Launcher runs.
/// Keeps the event loop going meanwhile (windows repaint, user input waits), so a slow start
/// doesn't freeze anything. False when the file never gets a key or its Launcher isn't running.
bool waitFor(const QString &file, int timeoutMs, const IsRunning &running = isRunning);

/// How long a new build gets to start: slow disks and virus scanners of new executables
/// take seconds (GH-3887)
const int DEFAULT_TIMEOUT_MS = 30000;
}
