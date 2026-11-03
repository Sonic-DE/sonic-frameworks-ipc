// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "frame.h"
#include "sonicipc_export.h"

#include <QList>
#include <QObject>

class QLocalServer;

namespace Sonic::Ipc {
class Connection;

/*!
 * \class Sonic::Ipc::Server
 * \inmodule SonicIpc
 * \inheaderfile Sonic/Ipc/Server
 *
 * \brief Listens on a local socket.
 *
 * Wraps each client in a Connection, parented to
 * this server and deleted when it disconnects.
 */
class SONICIPC_EXPORT Server : public QObject {
    Q_OBJECT

public:
    /*!
     * Who may connect to the socket.
     *
     * \value User Owner only (QLocalServer::UserAccessOption).
     * \value Group Owner and group.
     * \value World Everyone.
     */
    enum class Access {
        User,
        Group,
        World,
    };

    /*!
     * Creates a server that isn't listening yet, with the given \a parent.
     */
    explicit Server(QObject* parent = nullptr);
    ~Server() override;

    /*!
     * Passes \a nameOrPath to QLocalServer::listen() unchanged. Fails while
     * another server answers on it, so a live server is never replaced; a stale
     * socket file is removed and listened on.
     *
     * \a access says who may connect. Connections speak application protocol
     * \a protocolVersion and reject payloads larger than \a maxPayload bytes.
     */
    bool listen(const QString& nameOrPath, Access access, quint16 protocolVersion, qint64 maxPayload = defaultMaxPayload);
    /*!
     * The socket's file path; callers that hand the socket to another uid chown this after listen().
     */
    QString fullServerName() const;
    /*!
     * Why listen() failed.
     */
    QString errorString() const;
    /*!
     * Closes every connection and stops listening.
     */
    void close();

    /*!
     * Sends a frame of \a type with \a payload to every connection.
     */
    void broadcast(quint32 type, QByteArrayView payload);
    /*!
     * The connected clients.
     */
    QList<Connection*> connections() const;

    /*!
     * <RuntimeLocation>/\a fileName, or /tmp/sonic-<uid>/\a fileName when Qt has
     * no runtime location. The /tmp directory is created with mode 0700 and
     * refused (empty result) if it is a symlink, not a directory, owned by
     * another user, or open to group or others.
     */
    static QString userSocketPath(const QString& fileName);

Q_SIGNALS:
    /*!
     * A client connected; \a connection belongs to the server.
     */
    void newConnection(Sonic::Ipc::Connection* connection);

private:
    void acceptConnections();

    QLocalServer* m_server;
    QString m_errorString;
    QList<Connection*> m_connections;
    quint16 m_protocolVersion = 0;
    qint64 m_maxPayload = defaultMaxPayload;
};
}
