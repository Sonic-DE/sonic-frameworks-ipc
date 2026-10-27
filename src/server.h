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
 * Listens on a local socket and wraps each client in a Connection, parented to
 * this server and deleted when it disconnects.
 */
class SONICIPC_EXPORT Server : public QObject {
    Q_OBJECT

public:
    enum class Access {
        User, //!< owner only (UserAccessOption)
        Group, //!< owner and group
        World, //!< everyone
    };

    explicit Server(QObject* parent = nullptr);
    ~Server() override;

    /*!
     * Passes \a nameOrPath to QLocalServer::listen() unchanged. Fails while
     * another server answers on it, so a live server is never replaced; a stale
     * socket file is removed and listened on.
     */
    bool listen(const QString& nameOrPath, Access access, quint16 protocolVersion, qint64 maxPayload = defaultMaxPayload);
    //! The socket's file path; callers that hand the socket to another uid chown this after listen().
    QString fullServerName() const;
    QString errorString() const;
    void close();

    void broadcast(quint32 type, QByteArrayView payload);
    QList<Connection*> connections() const;

    /*!
     * <RuntimeLocation>/<fileName>, or /tmp/sonic-<uid>/<fileName> when Qt has
     * no runtime location. The /tmp directory is created with mode 0700 and
     * refused (empty result) if it is a symlink, not a directory, owned by
     * another user, or open to group or others.
     */
    static QString userSocketPath(const QString& fileName);

Q_SIGNALS:
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
