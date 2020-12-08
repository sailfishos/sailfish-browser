#include "logging.h"
#include <vault/unit.h>
#include <QProcess>
#include <QCoreApplication>
#include <QVariantList>
#include <QVariantMap>
#include <QDebug>
#include <QLoggingCategory>
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

const QVariantMap info = {
    {"home", QVariantMap({
                {"data", QVariantList({
                                  browser_dir + "/bookmarks.json"
                                })}
                , {"bin", QVariantList({
                                  moz_dir + "/key3.db"
                                , browser_dir + "/sailfish-browser.sqlite"
                                , cache_dir
                                , moz_dir + "/signons.sqlite"
                                })
                        }})}
    , {"options", QVariantMap({{"overwrite", true}})}
};
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
    return vault::unit::execute(info);
}
