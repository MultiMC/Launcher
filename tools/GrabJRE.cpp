#include <QTextStream>
#include <QCoreApplication>

#include "mojang/PackageManifest.h"
#include "mojang/PackageInstallTask.h"
#include <QCommandLineParser>

int main(int argc, char ** argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("GrabJRE");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Stupid thing that grabs a piston package and updates a local folder with it");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("url", "Source URL");
    parser.addPositionalArgument("version", "Source version");
    parser.addPositionalArgument("destination", "Destination folder to update");

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 3) {
        parser.showHelp(1);
    }

    // Run like ./GrabJRE "https://launchermeta.mojang.com/v1/packages/a1c15cc788f8893fba7e988eb27404772f699a84/manifest.json" "1.8.0_202" "test"

    auto url = args[0];
    auto version = args[1];
    auto destination = args[2];
    PackageInstallTask installTask(
        Net::Mode::Online,
        version,
        url,
        destination
    );
    installTask.start();
    QCoreApplication::connect(&installTask, &PackageInstallTask::progress, [&](qint64 now, qint64 total) {
        static int percentage = 0;
        if(total > 0) {
            int newPercentage = (now * 100.0f) / double(total);
            if(newPercentage != percentage) {
                percentage = newPercentage;
                QTextStream(stdout) << "Downloading: " << percentage << "% done\n";
            }
        }
    });
    QCoreApplication::connect(&installTask, &PackageInstallTask::finished, [&]() {
        app.exit(installTask.wasSuccessful() ? 0 : 1);
    });
    app.exec();
    return 0;
}
