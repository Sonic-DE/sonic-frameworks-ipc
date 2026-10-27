// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "server.h"
#include "connection.h"
#include "ipc_p.h"

#include <Sonic/Logging/Log>

#include <QDir>
#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>

#include <cerrno>
#include <sys/stat.h>
#include <unistd.h>

namespace Sonic::Ipc {
namespace {
    struct UserSocketTesting {
        bool emptyRuntimeLocation = false;
        QString temporaryDirectory;
    };

    UserSocketTesting& userSocketTesting()
    {
        static UserSocketTesting testing;
        return testing;
    }

    QLocalServer::SocketOptions socketOptions(Server::Access access)
    {
        switch (access) {
        case Server::Access::User:
            return QLocalServer::UserAccessOption;
        case Server::Access::Group:
            return QLocalServer::GroupAccessOption | QLocalServer::UserAccessOption;
        case Server::Access::World:
            return QLocalServer::WorldAccessOption;
        }
        return QLocalServer::UserAccessOption;
    }

    // True when a server answers on the name; such a socket must never be removed.
    bool somebodyListens(const QString& nameOrPath)
    {
        QLocalSocket probe;
        probe.connectToServer(nameOrPath);
        const bool live = probe.waitForConnected(500);
        probe.abort();
        return live;
    }
}

Server::Server(QObject* parent)
    : QObject(parent)
    , m_server(new QLocalServer(this))
{
    connect(m_server, &QLocalServer::newConnection, this, &Server::acceptConnections);
}

Server::~Server()
{
    // Connected sockets must go before the QLocalServer that created them.
    close();
    delete m_server;
}

bool Server::listen(const QString& nameOrPath, Access access, quint16 protocolVersion, qint64 maxPayload)
{
    m_protocolVersion = protocolVersion;
    m_maxPayload = maxPayload;
    m_errorString.clear();
    // With access options set, Qt binds in a temporary directory and renames over the path, replacing a live
    // socket without reporting AddressInUseError; so ask first.
    if (somebodyListens(nameOrPath)) {
        m_errorString = QStringLiteral("another server is listening on %1").arg(nameOrPath);
        sonicWarning() << "cannot listen:" << m_errorString;
        return false;
    }
    m_server->setSocketOptions(socketOptions(access));
    bool listening = m_server->listen(nameOrPath);
    if (!listening && m_server->serverError() == QAbstractSocket::AddressInUseError && !somebodyListens(nameOrPath)) {
        sonicInfo() << "removing the stale socket" << nameOrPath;
        QLocalServer::removeServer(nameOrPath);
        listening = m_server->listen(nameOrPath);
    }
    if (!listening) {
        sonicWarning() << "cannot listen on" << nameOrPath << ":" << m_server->errorString();
        return false;
    }
    sonicInfo() << "listening on" << m_server->fullServerName();
    return true;
}

QString Server::fullServerName() const
{
    return m_server->fullServerName();
}

QString Server::errorString() const
{
    return m_errorString.isEmpty() ? m_server->errorString() : m_errorString;
}

void Server::close()
{
    const QList<Connection*> connections = m_connections;
    m_connections.clear();
    for (Connection* connection : connections) {
        // Detach first: abort() emits disconnected, which would also schedule a delete.
        disconnect(connection, nullptr, this, nullptr);
        connection->socket()->abort();
        delete connection;
    }
    m_server->close();
}

void Server::broadcast(quint32 type, QByteArrayView payload)
{
    for (Connection* connection : std::as_const(m_connections)) {
        connection->send(type, payload);
    }
}

QList<Connection*> Server::connections() const
{
    return m_connections;
}

void Server::acceptConnections()
{
    while (QLocalSocket* socket = m_server->nextPendingConnection()) {
        auto* connection = new Connection(socket, m_protocolVersion, m_maxPayload, this);
        m_connections.append(connection);
        connect(connection, &Connection::disconnected, this, [this, connection] {
            m_connections.removeOne(connection);
            connection->deleteLater();
        });
        Q_EMIT newConnection(connection);
    }
}

QString Server::userSocketPath(const QString& fileName)
{
    const UserSocketTesting& testing = userSocketTesting();
    const QString runtime = testing.emptyRuntimeLocation ? QString() : QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (!runtime.isEmpty()) {
        return QDir(runtime).filePath(fileName);
    }

    const QString base = testing.temporaryDirectory.isEmpty() ? QStringLiteral("/tmp") : testing.temporaryDirectory;
    const QString directory = base + QStringLiteral("/sonic-") + QString::number(getuid());
    const QByteArray encoded = QFile::encodeName(directory);
    if (mkdir(encoded.constData(), 0700) != 0 && errno != EEXIST) {
        sonicWarning() << "cannot create" << directory << ":" << strerror(errno);
        return {};
    }
    struct stat st;
    if (lstat(encoded.constData(), &st) != 0 || !S_ISDIR(st.st_mode) || st.st_uid != getuid() || (st.st_mode & 077) != 0) {
        sonicWarning() << "refusing" << directory << "for the socket: it must be a directory of this user with mode 0700";
        return {};
    }
    return directory + QLatin1Char('/') + fileName;
}

namespace Internal {
    void setUserSocketFallbackForTesting(bool emptyRuntimeLocation, const QString& temporaryDirectory)
    {
        userSocketTesting() = {emptyRuntimeLocation, temporaryDirectory};
    }
}
}
