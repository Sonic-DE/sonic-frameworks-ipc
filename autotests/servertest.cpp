// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "server.h"
#include "client.h"
#include "connection.h"
#include "ipc_p.h"

#include <QDir>
#include <QFile>
#include <QLocalSocket>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <QTimer>
#include <QtConcurrentRun>

#include <memory>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

using namespace Sonic::Ipc;

namespace {
// Echoes every frame back with type + 100.
void echoFrames(Server& server)
{
    QObject::connect(&server, &Server::newConnection, &server, [](Connection* connection) {
        QObject::connect(connection, &Connection::frameReceived, connection, [connection](quint32 type, const QByteArray& payload) {
            connection->send(type + 100, payload);
        });
    });
}

// QLocalSocket may connect, and Client emit connected, inside connectToServer() itself.
Connection* connectClient(Client& client, const QString& path)
{
    QSignalSpy connected(&client, &Client::connected);
    client.connectToServer(path);
    if (connected.isEmpty() && !connected.wait(5000)) {
        return nullptr;
    }
    return connected.first().first().value<Connection*>();
}

mode_t modeOf(const QString& path)
{
    struct stat st;
    return lstat(QFile::encodeName(path).constData(), &st) == 0 ? (st.st_mode & 07777) : 0;
}
}

class ServerTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        // QSignalSpy stores signal arguments as QVariants.
        qRegisterMetaType<Sonic::Ipc::Connection*>();
    }

    void init()
    {
        QVERIFY(m_dir.isValid());
        m_path = m_dir.filePath(QStringLiteral("socket-%1").arg(m_counter++));
    }

    void cleanup()
    {
        Internal::setUserSocketFallbackForTesting(false, QString());
    }

    void requestAndResponse()
    {
        Server server;
        echoFrames(server);
        QVERIFY(server.listen(m_path, Server::Access::User, 1));
        Client client(1);
        Connection* connection = connectClient(client, m_path);
        QVERIFY(connection);
        QSignalSpy frames(connection, &Connection::frameReceived);
        QVERIFY(connection->send(5, "hello"));
        QVERIFY(frames.wait(5000));
        QCOMPARE(frames.first().at(0).toUInt(), 105u);
        QCOMPARE(frames.first().at(1).toByteArray(), QByteArray("hello"));
    }

    void broadcastToTwoClients()
    {
        Server server;
        QVERIFY(server.listen(m_path, Server::Access::User, 1));
        Client first(1);
        Client second(1);
        Connection* a = connectClient(first, m_path);
        Connection* b = connectClient(second, m_path);
        QVERIFY(a && b);
        QTRY_COMPARE(server.connections().size(), 2);
        QSignalSpy aFrames(a, &Connection::frameReceived);
        QSignalSpy bFrames(b, &Connection::frameReceived);
        server.broadcast(11, "to everyone");
        QTRY_COMPARE(aFrames.size(), 1);
        QTRY_COMPARE(bFrames.size(), 1);
        QCOMPARE(bFrames.first().at(1).toByteArray(), QByteArray("to everyone"));
    }

    void peerCredentials()
    {
        Server server;
        QVERIFY(server.listen(m_path, Server::Access::User, 1));
        Client client(1);
        QVERIFY(connectClient(client, m_path));
        QTRY_COMPARE(server.connections().size(), 1);
        const auto credentials = server.connections().first()->peerCredentials();
        QVERIFY(credentials);
        QCOMPARE(credentials->uid, qint64(getuid()));
        QCOMPARE(credentials->pid, qint64(getpid()));
    }

    void protocolErrorDisconnectsPeer()
    {
        Server server;
        QVERIFY(server.listen(m_path, Server::Access::User, 1));
        QSignalSpy connections(&server, &Server::newConnection);
        QLocalSocket raw;
        raw.connectToServer(m_path);
        QVERIFY(raw.waitForConnected(5000));
        QVERIFY(connections.wait(5000));
        auto* connection = connections.first().first().value<Connection*>();
        QSignalSpy errors(connection, &Connection::protocolError);
        QSignalSpy frames(connection, &Connection::frameReceived);
        raw.write(QByteArray(32, 'x'));
        raw.flush();
        QVERIFY(errors.wait(5000));
        QTRY_COMPARE(raw.state(), QLocalSocket::UnconnectedState);
        QCOMPARE(frames.size(), 0);
    }

    // Qt gives a UserAccessOption socket mode 0700; what matters is that group and others get nothing.
    void userAccessSocketMode()
    {
        Server server;
        QVERIFY(server.listen(m_path, Server::Access::User, 1));
        const mode_t mode = modeOf(server.fullServerName());
        QCOMPARE(mode & 077, mode_t(0));
        QCOMPARE(mode & 0600, mode_t(0600));
    }

    void chownKeepsModeAndAccess()
    {
        Server server;
        QVERIFY(server.listen(m_path, Server::Access::User, 1));
        const mode_t before = modeOf(server.fullServerName());
        QCOMPARE(chown(QFile::encodeName(server.fullServerName()).constData(), getuid(), getgid()), 0);
        QCOMPARE(modeOf(server.fullServerName()), before);
        QCOMPARE(before & 077, mode_t(0));
        Client client(1);
        QVERIFY(connectClient(client, server.fullServerName()));
    }

    void staleSocketRecovery()
    {
        // A socket file nobody listens on, as a crash leaves behind.
        const int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        sockaddr_un address{};
        address.sun_family = AF_UNIX;
        const QByteArray encoded = QFile::encodeName(m_path);
        memcpy(address.sun_path, encoded.constData(), encoded.size());
        QCOMPARE(bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)), 0);
        close(fd);
        QVERIFY(QFile::exists(m_path));

        Server server;
        QVERIFY(server.listen(m_path, Server::Access::User, 1));

        // A live server is never removed.
        Server second;
        QVERIFY(!second.listen(m_path, Server::Access::User, 1));
        Client client(1);
        QVERIFY(connectClient(client, m_path));
    }

    void blockingRequestKeepsEventLoopResponsive()
    {
        Server server;
        echoFrames(server);
        QVERIFY(server.listen(m_path, Server::Access::User, 1));

        int ticks = 0;
        QTimer heartbeat;
        connect(&heartbeat, &QTimer::timeout, this, [&ticks] {
            ++ticks;
        });
        heartbeat.start(10);

        const QString path = m_path;
        QFuture<std::optional<Frame>> future = QtConcurrent::run([path] {
            QString error;
            QThread::msleep(50);
            return Client::request(path, 7, "blocking", 1, 5000, &error);
        });
        QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(), 10000);
        const std::optional<Frame> reply = future.result();
        QVERIFY(reply);
        QCOMPARE(reply->type, 107u);
        QCOMPARE(reply->payload, QByteArray("blocking"));
        QVERIFY(ticks >= 2);
    }

    void userSocketPathUsesRuntimeLocation()
    {
        QTemporaryDir runtime;
        QFile::setPermissions(runtime.path(), QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
        qputenv("XDG_RUNTIME_DIR", QFile::encodeName(runtime.path()));
        QCOMPARE(Server::userSocketPath(QStringLiteral("x.sock")), runtime.path() + QStringLiteral("/x.sock"));
    }

    void userSocketPathFallback()
    {
        const QString base = m_dir.filePath(QStringLiteral("tmp-%1").arg(m_counter));
        QVERIFY(QDir().mkpath(base));
        Internal::setUserSocketFallbackForTesting(true, base);
        const QString directory = base + QStringLiteral("/sonic-") + QString::number(getuid());
        QCOMPARE(Server::userSocketPath(QStringLiteral("x.sock")), directory + QStringLiteral("/x.sock"));
        QCOMPARE(modeOf(directory), mode_t(0700));
    }

    void userSocketPathRefusesUnsafeDirectories_data()
    {
        QTest::addColumn<QString>("kind");
        QTest::newRow("symlink") << QStringLiteral("symlink");
        QTest::newRow("mode 0755") << QStringLiteral("open");
        QTest::newRow("regular file") << QStringLiteral("file");
    }

    void userSocketPathRefusesUnsafeDirectories()
    {
        QFETCH(QString, kind);
        const QString base = m_dir.filePath(QStringLiteral("unsafe-%1").arg(m_counter));
        QVERIFY(QDir().mkpath(base));
        const QString directory = base + QStringLiteral("/sonic-") + QString::number(getuid());
        if (kind == QLatin1String("symlink")) {
            QVERIFY(QDir().mkpath(base + QStringLiteral("/target")));
            QVERIFY(QFile::link(base + QStringLiteral("/target"), directory));
        } else if (kind == QLatin1String("open")) {
            QVERIFY(QDir().mkpath(directory));
            chmod(QFile::encodeName(directory).constData(), 0755);
        } else {
            QFile file(directory);
            QVERIFY(file.open(QIODevice::WriteOnly));
        }
        Internal::setUserSocketFallbackForTesting(true, base);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("refusing")));
        QVERIFY(Server::userSocketPath(QStringLiteral("x.sock")).isEmpty());
    }

    void versionRejection()
    {
        Server server;
        QVERIFY(server.listen(m_path, Server::Access::User, 1));
        QSignalSpy serverConnections(&server, &Server::newConnection);
        Client client(2);
        Connection* connection = connectClient(client, m_path);
        QVERIFY(connection);
        QTRY_COMPARE(serverConnections.size(), 1);
        auto* serverSide = serverConnections.first().first().value<Connection*>();
        QSignalSpy serverErrors(serverSide, &Connection::protocolError);
        QSignalSpy serverFrames(serverSide, &Connection::frameReceived);
        QSignalSpy rejected(connection, &Connection::versionRejected);
        QSignalSpy clientRejected(&client, &Client::versionRejected);
        QSignalSpy clientFrames(connection, &Connection::frameReceived);

        connection->send(1, "hello from version 2");
        QVERIFY(rejected.wait(5000));
        QCOMPARE(rejected.first().first().value<quint16>(), quint16(1));
        QCOMPARE(clientRejected.size(), 1);
        QCOMPARE(serverErrors.size(), 1);
        const QString reason = serverErrors.first().first().toString();
        QVERIFY(reason.contains(QLatin1String("2")) && reason.contains(QLatin1String("1")));
        QCOMPARE(serverFrames.size(), 0);
        QCOMPARE(clientFrames.size(), 0);
        QTRY_COMPARE(server.connections().size(), 0);
    }

    void reconnectAfterServerRestart()
    {
        auto server = std::make_unique<Server>();
        QVERIFY(server->listen(m_path, Server::Access::User, 1));
        Client client(1);
        client.setReconnectInterval(50);
        QSignalSpy connected(&client, &Client::connected);
        client.connectToServer(m_path);
        QTRY_COMPARE(connected.size(), 1);

        server.reset();
        server = std::make_unique<Server>();
        echoFrames(*server);
        QVERIFY(server->listen(m_path, Server::Access::User, 1));
        QTRY_COMPARE_WITH_TIMEOUT(connected.size(), 2, 5000);

        Connection* connection = client.connection();
        QVERIFY(connection);
        QSignalSpy frames(connection, &Connection::frameReceived);
        connection->send(3, "again");
        QVERIFY(frames.wait(5000));
        QCOMPARE(frames.first().at(0).toUInt(), 103u);
    }

    void rejectedClientStopsReconnecting()
    {
        Server server;
        QVERIFY(server.listen(m_path, Server::Access::User, 1));
        Client client(2);
        client.setReconnectInterval(50);
        QSignalSpy connected(&client, &Client::connected);
        QSignalSpy failed(&client, &Client::connectionFailed);
        QSignalSpy rejected(&client, &Client::versionRejected);
        client.connectToServer(m_path);
        QTRY_COMPARE(connected.size(), 1);
        client.connection()->send(1, "x");
        QVERIFY(rejected.wait(5000));
        QTest::qWait(500);
        QCOMPARE(connected.size(), 1);
        QCOMPARE(failed.size(), 0);
    }

    void disconnectFromServerStopsReconnecting()
    {
        auto server = std::make_unique<Server>();
        QVERIFY(server->listen(m_path, Server::Access::User, 1));
        Client client(1);
        client.setReconnectInterval(50);
        QSignalSpy connected(&client, &Client::connected);
        client.connectToServer(m_path);
        QTRY_COMPARE(connected.size(), 1);
        client.disconnectFromServer();
        server.reset();
        server = std::make_unique<Server>();
        QVERIFY(server->listen(m_path, Server::Access::User, 1));
        QTest::qWait(300);
        QCOMPARE(connected.size(), 1);
    }

private:
    QTemporaryDir m_dir;
    QString m_path;
    int m_counter = 0;
};

QTEST_GUILESS_MAIN(ServerTest)

#include "servertest.moc"
