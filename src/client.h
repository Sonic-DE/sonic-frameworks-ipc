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
 * Connects to a Server. Long-lived subscribers can turn on automatic
 * reconnection; it stops for good once the server rejects this side's
 * protocol version.
 */
class SONICIPC_EXPORT Client : public QObject {
    Q_OBJECT

public:
    explicit Client(quint16 protocolVersion, qint64 maxPayload = defaultMaxPayload, QObject* parent = nullptr);
    ~Client() override;

    void connectToServer(const QString& nameOrPath);
    //! Reconnects this long after a failed connect or a disconnect; 0, the default, turns it off.
    void setReconnectInterval(int msecs);
    //! Stops reconnecting and closes the connection.
    void disconnectFromServer();
    Connection* connection() const;

    /*!
     * Blocking request for threads without an event loop: connects, sends one
     * frame and waits for one reply, all within \a timeoutMs (-1: no timeout).
     * The socket is created on the calling thread.
     */
    static std::optional<Frame> request(const QString& nameOrPath,
        quint32 type,
        QByteArrayView payload,
        quint16 protocolVersion,
        int timeoutMs,
        QString* error,
        qint64 maxPayload = defaultMaxPayload);

Q_SIGNALS:
    void connected(Sonic::Ipc::Connection* connection);
    void connectionFailed(const QString& reason);
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
