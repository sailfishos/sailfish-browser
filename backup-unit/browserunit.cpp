#include "logging.h"
#include <vault/unit.h>
#include <QProcess>
#include <QCoreApplication>
#include <QVariantList>
#include <QVariantMap>
#include <QDebug>
#include <QLoggingCategory>
#include <QFileInfo>
#include <QDir>
#include <sys/types.h>
#include <signal.h>
#include <set>
#include <unistd.h>
#include <stdexcept>

void stop_browser()
{
    qCDebug(lcBackupLog) << "Terminating browser";
    QProcess ps;
    auto get_browser_pids = [&ps]() { 
        std::set<int> res;
        ps.execute("pgrep", {"-f", "sailfish-browser"});
        if (ps.exitStatus() == QProcess::CrashExit) {
            qCDebug(lcBackupLog) << "pgrep failed";
            return res;
        }

        auto data = QString(ps.readAllStandardOutput()).split("\n");
        for (auto const &line : data) {
            if (!line.length())
                continue;

            bool ok = false;
            auto pid = line.toInt(&ok);
            if (ok)
                res.insert(pid);
        }
        return res;
    };

    auto const sec0_1 = 100000;

    auto sig = SIGINT;
    auto counter = 31;

    auto pids = get_browser_pids();
    for (; pids.size(); pids = get_browser_pids()) {
        bool is_running_left = false;
        for (auto const &pid : pids) {
            if (!kill(pid, sig)) {
                qCDebug(lcBackupLog) << "Sent" << sig << "to" << pid;
                is_running_left = true;
            }
        }
        if (!is_running_left)
            break;
        usleep(sec0_1);

        if (!--counter)
            throw std::runtime_error("Can't interrupt sailfish-browser");
        else if (counter < 6)
            sig = SIGKILL;
        else if (counter < 15)
            sig = SIGTERM;
    }
}

namespace {

const QString browser_dir = ".local/share/org.sailfishos/browser";
const QString moz_dir = browser_dir + "/.mozilla";
// thumbs
const QString cache_dir = ".cache/org.sailfishos/browser";
// files
const QString bookmarks = "/bookmarks.json";
const QString database = "/sailfish-browser.sqlite";
const QString keys = "/key3.db";
const QString signons = "/signons.sqlite";

const QVariantMap info = {
    {"home", QVariantMap({
                {"data", QVariantList({
                                  browser_dir + bookmarks
                                })}
                , {"bin", QVariantList({
                                  moz_dir + keys
                                , browser_dir + database
                                , cache_dir
                                , moz_dir + signons
                                })
                        }})}
    , {"options", QVariantMap({{"overwrite", true}})}
};

// old paths
const QString old_browser_dir = ".local/share/org.sailfishos/sailfish-browser";
const QString old_moz_dir = ".mozilla/mozembed";
const QString old_cache_dir = ".cache/org.sailfishos/sailfish-browser";

}

void fix(QString common, QString file, QString oldPath, QString newPath)
{
    const auto pathTemplate = QStringLiteral("%1/%3/%2").arg(common).arg(file);
    QFileInfo source(pathTemplate.arg(oldPath));
    if (source.exists()) {
        QFileInfo dest(pathTemplate.arg(newPath));
        dest.dir().mkpath(".");
        if (!source.dir().rename(source.fileName(), dest.absoluteFilePath())) {
            qCWarning(lcBackupLog) << "Moving" << source.filePath()
                                   << "to" << dest.absoluteFilePath() << "failed";
        }
    }
}

void fix_dir(QString common, QString oldPath, QString newPath)
{
    if (QFileInfo(common + "/" + oldPath).exists()) {
        if (!QDir(common).rename(oldPath, newPath)) {
            qCWarning(lcBackupLog) << "Moving" << oldPath
                                   << "to" << newPath << "failed";
        }
    }
}

void fix_import()
{
    // Check for old style backup and convert to current structure if needed
    fix(vault::unit::optValue("dir"), bookmarks, old_browser_dir, browser_dir);
    auto blobDir = vault::unit::optValue("bin-dir");
    fix(blobDir, database, old_browser_dir, browser_dir);
    fix(blobDir, keys, old_moz_dir, moz_dir);
    fix(blobDir, signons, old_moz_dir, moz_dir);
    fix_dir(blobDir, old_cache_dir, cache_dir);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    try {
        stop_browser();
    } catch (std::exception const &e) {
        qCDebug(lcBackupLog) << e.what();
        return 1;
    }
    if (vault::unit::optValue("action") == "import") {
        fix_import();
    }
    return vault::unit::execute(info);
}
