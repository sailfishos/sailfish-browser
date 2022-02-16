/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2021 Open Mobile Platform LLC.
 * Copyright (c) 2022 Jolla Ltd.
 */

#include <QtTest>
#include <QStandardPaths>
#include <webengine.h>
#include <webenginesettings.h>

#include "declarativeloginmodel.h"
#include "loginfiltermodel.h"

#define PROPAGATE_DELAY (50)

class tst_logins : public QObject
{
    Q_OBJECT

public:
    tst_logins();

private slots:
    // Maintenance
    void initTestCase();
    void init();
    void cleanupTestCase();

    // Test cases
    void testModelCreation();
    void testRemoveEntries();
    void testModifyEntriesWithoutCollisions();
    void testModifyEntriesWithCollisions();
    void testMixedActions();
    void testSignals();
    void testFiltering();

private:
    // Utils
    void addLogin(const QString &hostname, const QString &username, const QString &password);
    void removeAllLogins();
    void debugPrintLogins();
    void resetLoginModel();
    void populateLoginModel(int entries);

    DeclarativeLoginModel *loginModel;
};

tst_logins::tst_logins()
    : QObject()
    , loginModel(nullptr)
{
}

void tst_logins::initTestCase()
{
    SailfishOS::WebEngine *engine = SailfishOS::WebEngine::instance();

    if (!engine->isInitialized()) {
        QSignalSpy initializedSpy(engine, SIGNAL(initialized()));
        initializedSpy.wait();
        QCOMPARE(initializedSpy.count(), 1);
    }
}

void tst_logins::init() {
    removeAllLogins();
    if (loginModel) {
        delete loginModel;
        loginModel = nullptr;
    }
}

void tst_logins::cleanupTestCase()
{
    QTest::qWait(2000);

    SailfishOS::WebEngine::instance()->stopEmbedding();

    QTest::qWait(2000);
}

void tst_logins::addLogin(const QString &hostname, const QString &username, const QString &password)
{
    QVariantMap loginData;
    loginData.insert(QStringLiteral("hostname"), hostname);
    loginData.insert(QStringLiteral("formSubmitURL"), "");
    loginData.insert(QStringLiteral("httpRealm"), QString());
    loginData.insert(QStringLiteral("username"), username);
    loginData.insert(QStringLiteral("password"), password);
    loginData.insert(QStringLiteral("usernameField"), "");
    loginData.insert(QStringLiteral("passwordField"), "");

    LoginInfo login(loginData);

    QVariantMap data;
    data.insert(QStringLiteral("action"), "add");
    data.insert(QStringLiteral("newinfo"), login.toMap());

    SailfishOS::WebEngine::instance()->notifyObservers("embedui:logins", QVariant(data));
    // Wait to allow the async message to propagate
    QTest::qWait(PROPAGATE_DELAY);
}

void tst_logins::removeAllLogins()
{
    QVariantMap data;
    data.insert(QStringLiteral("action"), "removeAll");

    SailfishOS::WebEngine::instance()->notifyObservers("embedui:logins", QVariant(data));
    // Wait to allow the async message to propagate
    QTest::qWait(PROPAGATE_DELAY);
}

void tst_logins::debugPrintLogins()
{
    qDebug() << "Model entries:";
    for (int pos = 0; pos < loginModel->rowCount(); ++pos) {
        QModelIndex index = loginModel->index(pos);
        const int uid = loginModel->data(index, DeclarativeLoginModel::UidRole).toInt();
        const QString username = loginModel->data(index, DeclarativeLoginModel::UsernameRole).toString();
        const QString password = loginModel->data(index, DeclarativeLoginModel::PasswordRole).toString();
        qDebug() << "  Entry: " << pos << uid << username << password;
    }
}

void tst_logins::resetLoginModel()
{
    if (loginModel) {
        delete loginModel;
    }
    loginModel = new DeclarativeLoginModel();
    QSignalSpy countSpy(loginModel, SIGNAL(countChanged()));
    loginModel->componentComplete();
    countSpy.wait();
}

void tst_logins::populateLoginModel(int entries)
{
    // Initialise with multiple entries
    QVERIFY(entries > 1);
    const QString user("user%1");
    const QString pass("pass%1");
    for (int count = 0; count < entries; ++count) {
        addLogin("https://www.sailfishos.org", user.arg(count), pass.arg(count));
    }
    QTest::qWait(500);
}

void tst_logins::testModelCreation()
{
    // Empty model
    resetLoginModel();
    QCOMPARE(loginModel->rowCount(), 0);

    // Single entry
    addLogin("https://www.sailfishos.org", "testuser", "testpass");
    QTest::qWait(100);
    resetLoginModel();
    QCOMPARE(loginModel->rowCount(), 1);

    // Multiple entries
    const int entries = 100;
    populateLoginModel(entries);
    resetLoginModel();
    // One extra entry lingers from the single entry case
    QCOMPARE(loginModel->rowCount(), entries + 1);
}

void tst_logins::testRemoveEntries()
{
    const QString prefix("user");

    // Initialise with multiple entries
    const int entries = 100;
    populateLoginModel(entries);
    resetLoginModel();
    QCOMPARE(loginModel->rowCount(), entries);

    // Remove entries
    QSet<int> indicesToRemove({10, 14, 15, 18, 67, 31, 32, 33, 45, 44, 43});
    QList<int> uidsRemoved;
    QStringList usersRemoved;
    for (int index : indicesToRemove) {
        // Sanity checks
        QVERIFY(index < entries);
        QVERIFY(index >= 0);
        QModelIndex modelIndex(loginModel->index(index));

        int uid = loginModel->data(modelIndex, DeclarativeLoginModel::UidRole).toInt();
        uidsRemoved.append(uid);
        usersRemoved.append(loginModel->data(modelIndex, DeclarativeLoginModel::UsernameRole).toString());
    }

    QCOMPARE(loginModel->rowCount(), entries);
    for (int uid : uidsRemoved)     {
        loginModel->remove(uid);
    }
    QCOMPARE(loginModel->rowCount(), entries - indicesToRemove.size());

    // Check the remaining entries are as expected
    for (int index = 0; index < loginModel->rowCount(); ++index) {
        QModelIndex modelIndex(loginModel->index(index));
        for (QString userRemoved : usersRemoved) {
            QString userPresent = loginModel->data(modelIndex, DeclarativeLoginModel::UsernameRole).toString();
            QVERIFY(userPresent != userRemoved);
            QCOMPARE(userPresent.left(prefix.length()), prefix);
        }
        for (int uidRemoved : uidsRemoved) {
            int uidPresent = loginModel->data(modelIndex, DeclarativeLoginModel::UsernameRole).toInt();
            QVERIFY(uidPresent >= 0);
            QVERIFY(uidPresent != uidRemoved);
        }
    }

    // Removing duplicate entries fails
    for (int uid : uidsRemoved) {
        loginModel->remove(uid);
    }

    // Count should be unchanged
    QCOMPARE(loginModel->rowCount(), entries - indicesToRemove.size());

    // Check the remaining entries are as expected
    for (int index = 0; index < loginModel->rowCount(); ++index) {
        QModelIndex modelIndex(loginModel->index(index));
        for (QString userRemoved : usersRemoved) {
            QString userPresent = loginModel->data(modelIndex, DeclarativeLoginModel::UsernameRole).toString();
            QVERIFY(userPresent != userRemoved);
            QCOMPARE(userPresent.left(prefix.length()), prefix);
        }
        for (int uidRemoved : uidsRemoved) {
            int uidPresent = loginModel->data(modelIndex, DeclarativeLoginModel::UsernameRole).toInt();
            QVERIFY(uidPresent >= 0);
            QVERIFY(uidPresent != uidRemoved);
        }
    }

    // Recreate the model and check the remaining entries are as expected
    resetLoginModel();
    QCOMPARE(loginModel->rowCount(), entries - indicesToRemove.size());

    for (int index = 0; index < loginModel->rowCount(); ++index) {
        QModelIndex modelIndex(loginModel->index(index));
        for (QString userRemoved : usersRemoved) {
            QString userPresent = loginModel->data(modelIndex, DeclarativeLoginModel::UsernameRole).toString();
            QVERIFY(userPresent != userRemoved);
            QCOMPARE(userPresent.left(prefix.length()), prefix);
        }
        for (int uidRemoved : uidsRemoved) {
            int uidPresent = loginModel->data(modelIndex, DeclarativeLoginModel::UsernameRole).toInt();
            QVERIFY(uidPresent >= 0);
            QVERIFY(uidPresent != uidRemoved);
        }
    }
}

void tst_logins::testModifyEntriesWithoutCollisions()
{
    QSet<QPair<QString, QString>> logins;
    QScopedPointer<QSet<QPair<QString, QString>>> remainingLogins(nullptr);

    // Initialise with multiple entries
    const int entries = 10;
    populateLoginModel(entries);
    resetLoginModel();
    QCOMPARE(loginModel->rowCount(), entries);

    for (int index = 0; index < loginModel->rowCount(); ++index) {
        QString username = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UsernameRole).toString();
        QString password = loginModel->data(loginModel->index(index), DeclarativeLoginModel::PasswordRole).toString();
        logins.insert(QPair<QString, QString>(username, password));
    }

    // Modify entries without collisions
    QSet<int> indicesToModify({3, 5, 6});
    const QString userPrefix("freshname");
    const QString passPrefix("freshpass");
    const QString freshUser(userPrefix + "%1");
    const QString freshPass(passPrefix + "%1");
    int count = 0;
    for (int index : indicesToModify) {
        int uid = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UidRole).toInt();
        const QString oldUsername = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UsernameRole).toString();
        const QString oldPassword = loginModel->data(loginModel->index(index), DeclarativeLoginModel::PasswordRole).toString();

        QVERIFY(loginModel->canModify(uid, freshUser.arg(count), freshPass.arg(count)));
        loginModel->modify(uid, freshUser.arg(count), freshPass.arg(count));
        // Wait to allow the async message to propagate
        QTest::qWait(PROPAGATE_DELAY);

        // Expected change
        logins.remove(QPair<QString, QString>(oldUsername, oldPassword));
        logins.insert(QPair<QString, QString>(freshUser.arg(count), freshPass.arg(count)));

        count++;
    }

    // Check the modifications were successful
    QCOMPARE(loginModel->rowCount(), entries);
    // Make a copy
    remainingLogins.reset(new QSet<QPair<QString, QString>>(logins));
    for (int index = 0; index < loginModel->rowCount(); ++index) {
        const QString username = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UsernameRole).toString();
        const QString password = loginModel->data(loginModel->index(index), DeclarativeLoginModel::PasswordRole).toString();
        QVERIFY(remainingLogins->remove(QPair<QString, QString>(username, password)));
    }
    QVERIFY(remainingLogins->isEmpty());

    // Recreate the model and test again
    resetLoginModel();
    QCOMPARE(loginModel->rowCount(), entries);

    QCOMPARE(loginModel->rowCount(), entries);
    // Make a copy
    remainingLogins.reset(new QSet<QPair<QString, QString>>(logins));
    for (int index = 0; index < loginModel->rowCount(); ++index) {
        const QString username = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UsernameRole).toString();
        const QString password = loginModel->data(loginModel->index(index), DeclarativeLoginModel::PasswordRole).toString();
        QVERIFY(remainingLogins->remove(QPair<QString, QString>(username, password)));
    }
    QVERIFY(remainingLogins->isEmpty());
}

void tst_logins::testModifyEntriesWithCollisions()
{
    QSet<QPair<QString, QString>> logins;
    QScopedPointer<QSet<QPair<QString, QString>>> remainingLogins(nullptr);
    const QString user("user%1");
    const QString pass("pass%1");

    // Initialise with multiple entries
    const int entries = 10;
    populateLoginModel(entries);
    resetLoginModel();
    QCOMPARE(loginModel->rowCount(), entries);

    for (int index = 0; index < loginModel->rowCount(); ++index) {
        QString username = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UsernameRole).toString();
        QString password = loginModel->data(loginModel->index(index), DeclarativeLoginModel::PasswordRole).toString();
        logins.insert(QPair<QString, QString>(username, password));
    }

    // Modify entries without collisions
    QSet<int> indicesToModify({3, 5, 6});
    int count = 0;
    for (int index : indicesToModify) {
        int uid = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UidRole).toInt();
        const QString oldUsername = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UsernameRole).toString();
        const QString oldPassword = loginModel->data(loginModel->index(index), DeclarativeLoginModel::PasswordRole).toString();

        const QString newUsername = user.arg(count);
        const QString newPassword = pass.arg(count);

        QVERIFY(!loginModel->canModify(uid, newUsername, newPassword));
        // We can't make the modification, but we're going to try anyway
        // This should have no effect on the model
        loginModel->modify(uid, newUsername, newPassword);
        // Wait to allow the async message to propagate
        QTest::qWait(PROPAGATE_DELAY);

        count++;
    }

    // Check the modifications were successful
    QCOMPARE(loginModel->rowCount(), entries);
    // Make a copy
    remainingLogins.reset(new QSet<QPair<QString, QString>>(logins));
    for (int index = 0; index < loginModel->rowCount(); ++index) {
        const QString username = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UsernameRole).toString();
        const QString password = loginModel->data(loginModel->index(index), DeclarativeLoginModel::PasswordRole).toString();
        QVERIFY(remainingLogins->remove(QPair<QString, QString>(username, password)));
    }
    QVERIFY(remainingLogins->isEmpty());

    // Recreate the model and test again
    resetLoginModel();
    QCOMPARE(loginModel->rowCount(), entries);

    // Make a copy
    remainingLogins.reset(new QSet<QPair<QString, QString>>(logins));
    for (int index = 0; index < loginModel->rowCount(); ++index) {
        const QString username = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UsernameRole).toString();
        const QString password = loginModel->data(loginModel->index(index), DeclarativeLoginModel::PasswordRole).toString();
        QVERIFY(remainingLogins->remove(QPair<QString, QString>(username, password)));
    }
    QVERIFY(remainingLogins->isEmpty());
}

void tst_logins::testMixedActions()
{
    QSet<QPair<QString, QString>> logins;
    QScopedPointer<QSet<QPair<QString, QString>>> remainingLogins(nullptr);

    // Initialise with multiple entries
    const int entries = 30;
    QVERIFY(entries > 1);
    const QString user("user%1");
    const QString pass("testpass%1");
    int fresh = 0;
    for (int count = 0; count < entries; ++count) {
        addLogin("https://www.sailfishos.org", user.arg(fresh), pass.arg(fresh));
        logins.insert(QPair<QString, QString>(user.arg(fresh), pass.arg(fresh)));
        ++fresh;
    }
    resetLoginModel();
    QCOMPARE(loginModel->rowCount(), logins.count());

    // Remove some entries
    QSet<int> indicesToRemove({4, 6, 8});
    for (int index : indicesToRemove) {
        QVERIFY(index >= 0);
        QVERIFY(index < logins.count());
        const int uid = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UidRole).toInt();
        const QString username = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UsernameRole).toString();
        const QString password = loginModel->data(loginModel->index(index), DeclarativeLoginModel::PasswordRole).toString();

        loginModel->remove(uid);
        // Wait to allow the async message to propagate
        QTest::qWait(PROPAGATE_DELAY);
        logins.remove(QPair<QString, QString>(username, password));
    }

    QCOMPARE(loginModel->rowCount(), logins.count());

    // Modify some entries
    QSet<int> indicesToModify({1, 11, 16});
    for (int index : indicesToModify) {
        QVERIFY(index >= 0);
        QVERIFY(index < logins.count());
        const int uid = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UidRole).toInt();
        const QString oldUsername = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UsernameRole).toString();
        const QString oldPassword = loginModel->data(loginModel->index(index), DeclarativeLoginModel::PasswordRole).toString();

        const QString newUsername = user.arg(fresh);
        const QString newPassword = pass.arg(fresh);

        loginModel->modify(uid, newUsername, newPassword);
        // Wait to allow the async message to propagate
        QTest::qWait(PROPAGATE_DELAY);
        logins.remove(QPair<QString, QString>(oldUsername, oldPassword));
        logins.insert(QPair<QString, QString>(newUsername, newPassword));

        ++fresh;
    }
    QCOMPARE(loginModel->rowCount(), logins.count());

    // Remove some more entries
    QSet<int> moreIndicesToRemove({0, 9, 17});
    for (int index : moreIndicesToRemove) {
        QVERIFY(index >= 0);
        QVERIFY(index < logins.count());
        const int uid = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UidRole).toInt();
        const QString username = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UsernameRole).toString();
        const QString password = loginModel->data(loginModel->index(index), DeclarativeLoginModel::PasswordRole).toString();

        loginModel->remove(uid);
        // Wait to allow the async message to propagate
        QTest::qWait(PROPAGATE_DELAY);
        logins.remove(QPair<QString, QString>(username, password));
    }
    QCOMPARE(loginModel->rowCount(), logins.count());

    // Modify some more entries
    QSet<int> moreIndicesToModify({9, 12, 15});
    for (int index : moreIndicesToModify) {
        QVERIFY(index >= 0);
        QVERIFY(index < logins.count());
        const int uid = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UidRole).toInt();
        const QString oldUsername = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UsernameRole).toString();
        const QString oldPassword = loginModel->data(loginModel->index(index), DeclarativeLoginModel::PasswordRole).toString();

        const QString newUsername = user.arg(fresh);
        const QString newPassword = pass.arg(fresh);

        loginModel->modify(uid, newUsername, newPassword);
        // Wait to allow the async message to propagate
        QTest::qWait(PROPAGATE_DELAY);
        logins.remove(QPair<QString, QString>(oldUsername, oldPassword));
        logins.insert(QPair<QString, QString>(newUsername, newPassword));

        ++fresh;
    }
    QCOMPARE(loginModel->rowCount(), logins.count());

    // Check the entries are as we expected them to be
    remainingLogins.reset(new QSet<QPair<QString, QString>>(logins));
    for (int index = 0; index < loginModel->rowCount(); ++index) {
        const QString username = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UsernameRole).toString();
        const QString password = loginModel->data(loginModel->index(index), DeclarativeLoginModel::PasswordRole).toString();
        QVERIFY(remainingLogins->remove(QPair<QString, QString>(username, password)));
    }
    QVERIFY(remainingLogins->isEmpty());

    // Recreate the model and test again
    resetLoginModel();
    QCOMPARE(loginModel->rowCount(), logins.count());

    // Make a copy
    remainingLogins.reset(new QSet<QPair<QString, QString>>(logins));
    for (int index = 0; index < loginModel->rowCount(); ++index) {
        const QString username = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UsernameRole).toString();
        const QString password = loginModel->data(loginModel->index(index), DeclarativeLoginModel::PasswordRole).toString();
        QVERIFY(remainingLogins->remove(QPair<QString, QString>(username, password)));
    }
    QVERIFY(remainingLogins->isEmpty());
}

void tst_logins::testSignals()
{
    // Initialise with multiple entries
    const int entries = 30;
    QVERIFY(entries > 1);
    const QString user("user%1");
    const QString pass("testpass%1");
    int fresh = 0;
    for (int count = 0; count < entries; ++count) {
        addLogin("https://www.sailfishos.org", user.arg(fresh), pass.arg(fresh));
        ++fresh;
    }
    resetLoginModel();
    QCOMPARE(loginModel->rowCount(), entries);

    QSignalSpy countSpy(loginModel, SIGNAL(countChanged()));
    QSignalSpy rowsRemovedSpy(loginModel, SIGNAL(rowsRemoved(QModelIndex,int,int)));
    QSignalSpy dataChangedSpy(loginModel, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)));

    // Find an element to work with
    const int index = 25;
    QVERIFY(index < entries);
    QVERIFY(index >= 0);
    const int uid = loginModel->data(loginModel->index(index), DeclarativeLoginModel::UidRole).toInt();

    // Modification signals
    loginModel->modify(uid, "anonymous", "none");
    dataChangedSpy.wait();
    QCOMPARE(rowsRemovedSpy.count(), 0);
    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(countSpy.count(), 0);

    // Removal signals
    loginModel->remove(uid);
    rowsRemovedSpy.wait();
    QCOMPARE(rowsRemovedSpy.count(), 1);
    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(countSpy.count(), 1);
}

void tst_logins::testFiltering()
{
    int uid;

    // Initialise with multiple entries
    const int entries = 30;

    // Initialise with multiple entries
    QVERIFY(entries > 1);
    const QString user("user%1");
    const QString pass("pass%1");
    const QString domain("https://www.sailfishos%1.org");
    for (int count = 0; count < entries; ++count) {
        addLogin(domain.arg(count), user.arg(count), pass.arg(count));
    }

    resetLoginModel();
    QCOMPARE(loginModel->rowCount(), entries);

    LoginFilterModel filterModel(this);
    filterModel.setSourceModel(loginModel);
    QCOMPARE(filterModel.rowCount(), entries);

    filterModel.setSearch("https://www.sailfishos");
    QCOMPARE(filterModel.rowCount(), entries);

    filterModel.setSearch("sailfishos1");
    QCOMPARE(filterModel.rowCount(), 11);

    filterModel.setSearch("fishos2");
    QCOMPARE(filterModel.rowCount(), 11);

    filterModel.setSearch("7");
    QCOMPARE(filterModel.rowCount(), 3);

    filterModel.setSearch("");
    QCOMPARE(filterModel.rowCount(), entries);

    filterModel.setSearch("sailfishos1");
    QCOMPARE(filterModel.rowCount(), 11);

    // Check modification affects the filtered model too
    QSignalSpy dataChangedSpy(&filterModel, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)));
    uid = loginModel->data(loginModel->index(11), DeclarativeLoginModel::UidRole).toInt();
    loginModel->modify(uid, "user20", "freshpass1");
    dataChangedSpy.wait();
    QCOMPARE(filterModel.rowCount(), 11);

    filterModel.setSearch("7");
    QCOMPARE(filterModel.rowCount(), 3);

    // Check removal affects the filtered model too
    QSignalSpy rowRemovedSpy(&filterModel, SIGNAL(rowsRemoved(QModelIndex,int,int)));
    uid = loginModel->data(loginModel->index(17), DeclarativeLoginModel::UidRole).toInt();
    loginModel->remove(uid);
    rowRemovedSpy.wait();
    QCOMPARE(filterModel.rowCount(), 2);
}

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(new QGuiApplication(argc, argv));
    QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    qDebug() << "Profile path: " << path;
    SailfishOS::WebEngine::initialize(path);
    SailfishOS::WebEngineSettings::initialize();

    tst_logins testcase;

    QString mozillaDir = QString("%1/.mozilla/").arg(path);
    QDir dir(mozillaDir);
    if (dir.exists()) {
        // Remove any existing profile
        dir.removeRecursively();
    }
    dir.mkpath(dir.path());
    QFile::copy("/usr/share/sailfish-browser/data/prefs.js", mozillaDir + "prefs.js");

    return QTest::qExec(&testcase, argc, argv);
}

#include "tst_logins.moc"
