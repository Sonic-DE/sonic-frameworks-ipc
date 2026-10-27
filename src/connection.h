// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "frame.h"
#include "framereader.h"
#include "sonicipc_export.h"

#include <QObject>

#include <optional>

class QLocalSocket;

namespace Sonic::Ipc {
/*!
 * One framed connection over a QLocalSocket. Frames arrive through
 * frameReceived(); threads without an event loop use waitForFrame() and
 * sendBlocking() instead. A protocol error aborts the socket.
 */
class SONICIPC_EXPORT Connection : public QObject {
    Q_OBJECT

public:
    struct PeerCredentials {
        qint64 uid = -1;
        qint64 gid = -1;
        qint64 pid = -1; //!< -1 where the platform doesn't report it (BSD)
    };

    //! Takes \a socket and reparents it to this connection.
    Connection(QLocalSocket* socket, quint16 protocolVersion, qint64 maxPayload = defaultMaxPayload, QObject* parent = nullptr);
    ~Connection() override;

    bool send(quint32 type, QByteArrayView payload);
    //! Writes, then waits until everything is written or \a msecs pass (-1: no timeout).
    bool sendBlocking(quint32 type, QByteArrayView payload, int msecs);
    //! Blocks until a frame arrives or \a msecs pass (-1: no timeout).
    std::optional<Frame> waitForFrame(int msecs);

    std::optional<PeerCredentials> peerCredentials() const;
    QLocalSocket* socket() const;
    quint16 protocolVersion() const;

Q_SIGNALS:
    void frameReceived(quint32 type, const QByteArray& payload);
    void protocolError(const QString& reason);
    //! The peer rejected this side's protocol version.
    void versionRejected(quint16 peerVersion);
    void disconnected();

private:
    void readAvailable();
    //! Returns the next application frame, handling library frames and errors; false when the connection was aborted.
    bool takeFrame(std::optional<Frame>* frame);
    void handleReaderError();

    QLocalSocket* m_socket;
    quint16 m_protocolVersion;
    FrameReader m_reader;
    bool m_blocking = false;
    bool m_failed = false;
};
}
