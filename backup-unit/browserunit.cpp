#include <vault/unit.hpp>
#include <qtaround/os.hpp>
#include <qtaround/subprocess.hpp>
#include <qtaround/error.hpp>
#include <qtaround/util.hpp>
#include <qtaround/debug.hpp>

#include <QCoreApplication>
#include <sys/types.h>
#include <signal.h>
#include <set>
#include <unistd.h>

namespace os = qtaround::os;
namespace subprocess = qtaround::subprocess;
namespace error = qtaround::error;
namespace debug = qtaround::debug;

void stop_browser()
{
    debug::info("Terminating browser");
    subprocess::Process ps;
    auto get_browser_pids = [&ps]() {
        std::set<int> res;
        ps.start("pgrep", {"-f", "sailfish-browser"});
        ps.wait(-1);
        if (ps.rc()) {
            debug::debug("No browser is running");
            return res;
        }
        auto data = str(ps.stdout()).split("\n");
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
            if (!::kill(pid, sig)) {
                debug::debug("Sent", sig, "to", pid);
                is_running_left = true;
            }
        }
        if (!is_running_left)
            break;

        ::usleep(sec0_1);

        if (!--counter)
            error::raise({{"msg", "Can't interrupt sailfish-browser"}});
        else if (counter < 6)
            sig = SIGKILL;
        else if (counter < 15)
            sig = SIGTERM;
    }
}

namespace {

const QString browser_dir = ".local/share/org.sailfishos/sailfish-browser";
const QString moz_dir = ".mozilla/mozembed";
// thumbs
const QString cache_dir = ".cache/org.sailfishos/sailfish-browser";

const QVariantMap info = {
    {"home", map({
                {"data", list({
                            os::path::join(browser_dir, "bookmarks.json")
                                })}
                , {"bin", list({
                            os::path::join(moz_dir, "key3.db")
                                , os::path::join(browser_dir, "sailfish-browser.sqlite")
                                , cache_dir
                                , os::path::join(moz_dir, "signons.sqlite")
                                })
                        }})}
    , {"options", map({{"overwrite", true}})}
};

}

int main(int argc, char *argv[])
{
    try {
        QCoreApplication app(argc, argv);
        using namespace vault::unit;
        stop_browser();
        execute(getopt(), info);
    } catch (error::Error const &e) {
        qDebug() << e;
        return 1;
    } catch (std::exception const &e) {
        qDebug() << e.what();
        return 2;
    }
    return 0;
}
