// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "frame.h"
#include "sonicipc_export.h"

#include <QObject>
#include <QString>

#include <optional>

class QLocalSocket;
class QTimer;

namespace Sonic::Ipc {
class Connection;

/*!
 * \class Sonic::Ipc::Client
 * \inmodule SonicIpc
 * \inheaderfile Sonic/Ipc/Client
 *
 * \brief Connects to a Server.
 *
 * Long-lived subscribers can turn on automatic
 * reconnection; it stops for good once the server rejects this side's
 * protocol version.
 */
class SONICIPC_EXPORT Client : public QObject {
    Q_OBJECT

public:
    /*!
     * Creates a client that speaks application protocol \a protocolVersion and
     * rejects payloads larger than \a maxPayload bytes, with the given \a parent.
     */
    explicit Client(quint16 protocolVersion, qint64 maxPayload = defaultMaxPayload, QObject* parent = nullptr);
    ~Client() override;

    /*!
     * Connects to the server at \a nameOrPath, as QLocalSocket::connectToServer()
     * takes it; connected() or connectionFailed() follows.
     */
    void connectToServer(const QString& nameOrPath);
    /*!
     * Reconnects \a msecs after a failed connect or a disconnect; 0, the default, turns it off.
     */
    void setReconnectInterval(int msecs);
    /*!
     * Stops reconnecting and closes the connection.
     */
    void disconnectFromServer();
    /*!
     * The current connection; null while not connected.
     */
    Connection* connection() const;

    /*!
     * Blocking request for threads without an event loop: connects to
     * \a nameOrPath, sends one frame of \a type with \a payload in protocol
     * \a protocolVersion and waits for one reply, all within \a timeoutMs
     * (-1: no timeout). Replies larger than \a maxPayload bytes are rejected.
     * The socket is created on the calling thread.
     *
     * On failure it returns nothing and sets \a error, when it isn't null.
     */
    static std::optional<Frame> request(const QString& nameOrPath,
        quint32 type,
        QByteArrayView payload,
        quint16 protocolVersion,
        int timeoutMs,
        QString* error,
        qint64 maxPayload = defaultMaxPayload);

Q_SIGNALS:
    /*!
     * The client connected; \a connection belongs to the client.
     */
    void connected(Sonic::Ipc::Connection* connection);
    /*!
     * Connecting failed, for the given \a reason.
     */
    void connectionFailed(const QString& reason);
    /*!
     * The server rejected this side's protocol version; it speaks
     * \a serverVersion. The client stops reconnecting.
     */
    void versionRejected(quint16 serverVersion);

private:
    void attempt();
    void scheduleReconnect();

    quint16 m_protocolVersion;
    qint64 m_maxPayload;
    QString m_name;
    QTimer* m_reconnectTimer;
    QLocalSocket* m_pendingSocket = nullptr;
    Connection* m_connection = nullptr;
    bool m_stopped = false;
    bool m_outageReported = false;
};
}
