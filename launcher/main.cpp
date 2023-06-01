#include "Application.h"

// #define BREAK_INFINITE_LOOP
// #define BREAK_EXCEPTION
// #define BREAK_RETURN

#ifdef BREAK_INFINITE_LOOP
#include <thread>
#include <chrono>
#endif

#if defined Q_OS_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <iostream>

namespace {
void earlyDebugOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    const char *levels = "DWCFIS";
    const QString format("EARLY %1 %2\n");
    QString out = format.arg(levels[type]).arg(msg);
    QTextStream(stderr) << out.toLocal8Bit();
    fflush(stderr);
}
}

int main(int argc, char *argv[])
{
#ifdef BREAK_INFINITE_LOOP
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
#endif
#ifdef BREAK_EXCEPTION
    throw 42;
#endif
#ifdef BREAK_RETURN
    return 42;
#endif

#if defined Q_OS_WIN32
    // used on Windows to attach the standard IO streams
    bool consoleAttached = false;
    // attach the parent console
    if(AttachConsole(ATTACH_PARENT_PROCESS))
    {
        // if attach succeeds, reopen and sync all the i/o
        if(freopen("CON", "w", stdout))
        {
            std::cout.sync_with_stdio();
        }
        if(freopen("CON", "w", stderr))
        {
            std::cerr.sync_with_stdio();
        }
        if(freopen("CON", "r", stdin))
        {
            std::cin.sync_with_stdio();
        }
        auto out = GetStdHandle (STD_OUTPUT_HANDLE);
        DWORD written;
        const char * endline = "\n";
        WriteConsole(out, endline, strlen(endline), &written, NULL);
        consoleAttached = true;
    }
#endif

    qInstallMessageHandler(earlyDebugOutput);

    // initialize Qt
    Application app(argc, argv);

    int result = 1;
    
    switch (app.status())
    {
    case Application::StartingUp:
    case Application::Initialized:
    {
        Q_INIT_RESOURCE(multimc);
        Q_INIT_RESOURCE(backgrounds);
        Q_INIT_RESOURCE(documents);
        Q_INIT_RESOURCE(logo);

        Q_INIT_RESOURCE(pe_dark);
        Q_INIT_RESOURCE(pe_light);
        Q_INIT_RESOURCE(pe_blue);
        Q_INIT_RESOURCE(pe_colored);
        Q_INIT_RESOURCE(OSX);
        Q_INIT_RESOURCE(iOS);
        Q_INIT_RESOURCE(flat);
        result = app.exec();
    }
    case Application::Failed:
        result = 1;
    case Application::Succeeded:
        result = 0;
    }
#if defined Q_OS_WIN32
    // Detach from Windows console
    if(consoleAttached)
    {
        fclose(stdout);
        fclose(stdin);
        fclose(stderr);
        FreeConsole();
    }
#endif
    return result;
}
