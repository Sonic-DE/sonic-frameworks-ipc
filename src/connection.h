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
 * \class Sonic::Ipc::Connection
 * \inmodule SonicIpc
 * \inheaderfile Sonic/Ipc/Connection
 *
 * \brief One framed connection over a QLocalSocket.
 *
 * Frames arrive through
 * frameReceived(); threads without an event loop use waitForFrame() and
 * sendBlocking() instead. A protocol error aborts the socket.
 */
class SONICIPC_EXPORT Connection : public QObject {
    Q_OBJECT

public:
    /*!
     * \class Sonic::Ipc::Connection::PeerCredentials
     * \inmodule SonicIpc
     * \inheaderfile Sonic/Ipc/Connection
     *
     * \brief The process at the other end of a connection.
     */
    struct PeerCredentials {
        /*!
         * \variable Sonic::Ipc::Connection::PeerCredentials::uid
         * The peer's user ID.
         */
        qint64 uid = -1;
        /*!
         * \variable Sonic::Ipc::Connection::PeerCredentials::gid
         * The peer's group ID.
         */
        qint64 gid = -1;
        /*!
         * \variable Sonic::Ipc::Connection::PeerCredentials::pid
         * The peer's process ID; -1 where the platform doesn't report it (BSD).
         */
        qint64 pid = -1;
    };

    /*!
     * Takes \a socket and reparents it to this connection, which speaks
     * application protocol \a protocolVersion and rejects payloads larger than
     * \a maxPayload bytes. \a parent is the connection's parent.
     */
    Connection(QLocalSocket* socket, quint16 protocolVersion, qint64 maxPayload = defaultMaxPayload, QObject* parent = nullptr);
    ~Connection() override;

    /*!
     * Writes a frame of \a type with \a payload; false when the socket isn't
     * connected, the write fails, or \a type is the reserved
     * Frame::VersionRejected.
     */
    bool send(quint32 type, QByteArrayView payload);
    /*!
     * Writes a frame of \a type with \a payload, then waits until everything is
     * written or \a msecs pass (-1: no timeout).
     */
    bool sendBlocking(quint32 type, QByteArrayView payload, int msecs);
    /*!
     * Blocks until a frame arrives or \a msecs pass (-1: no timeout).
     */
    std::optional<Frame> waitForFrame(int msecs);

    /*!
     * The process at the other end; nothing when the socket has no descriptor
     * or the system doesn't tell.
     */
    std::optional<PeerCredentials> peerCredentials() const;
    /*!
     * The connection's socket.
     */
    QLocalSocket* socket() const;
    /*!
     * The application protocol version this side speaks.
     */
    quint16 protocolVersion() const;

Q_SIGNALS:
    /*!
     * A frame of \a type with \a payload arrived.
     */
    void frameReceived(quint32 type, const QByteArray& payload);
    /*!
     * The peer sent data this side rejects, for the given \a reason; the
     * socket is aborted. A peer with another protocol version is told so first.
     */
    void protocolError(const QString& reason);
    /*!
     * The peer rejected this side's protocol version; it speaks \a peerVersion.
     */
    void versionRejected(quint16 peerVersion);
    /*!
     * The socket disconnected.
     */
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
