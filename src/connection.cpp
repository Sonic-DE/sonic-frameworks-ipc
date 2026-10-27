// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "connection.h"
#include "ipc_p.h"

#include <Sonic/Logging/Log>

#include <QDeadlineTimer>
#include <QLocalSocket>
#include <QPointer>
#include <QtEndian>

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace Sonic::Ipc {
Connection::Connection(QLocalSocket* socket, quint16 protocolVersion, qint64 maxPayload, QObject* parent)
    : QObject(parent)
    , m_socket(socket)
    , m_protocolVersion(protocolVersion)
    , m_reader(protocolVersion, maxPayload)
{
    m_socket->setParent(this);
    // Direct connections: QLocalSocket's signal argument types aren't registered metatypes.
    connect(m_socket, &QLocalSocket::readyRead, this, &Connection::readAvailable, Qt::DirectConnection);
    connect(m_socket, &QLocalSocket::disconnected, this, &Connection::disconnected, Qt::DirectConnection);
}

Connection::~Connection() = default;

bool Connection::send(quint32 type, QByteArrayView payload)
{
    const QByteArray frame = encodeFrame(type, payload, m_protocolVersion);
    if (frame.isEmpty() || m_socket->state() != QLocalSocket::ConnectedState) {
        return false;
    }
    sonicDebug() << "sending frame type" << type << "with" << payload.size() << "bytes";
    if (m_socket->write(frame) != frame.size()) {
        sonicWarning() << "writing a frame failed:" << m_socket->errorString();
        return false;
    }
    m_socket->flush();
    return true;
}

bool Connection::sendBlocking(quint32 type, QByteArrayView payload, int msecs)
{
    if (!send(type, payload)) {
        return false;
    }
    const QDeadlineTimer deadline(msecs < 0 ? QDeadlineTimer(QDeadlineTimer::Forever) : QDeadlineTimer(msecs));
    while (m_socket->bytesToWrite() > 0) {
        if (!m_socket->waitForBytesWritten(deadline.isForever() ? -1 : int(deadline.remainingTime()))) {
            return false;
        }
    }
    return true;
}

std::optional<Frame> Connection::waitForFrame(int msecs)
{
    // readyRead fires inside waitForReadyRead; in blocking mode it only feeds the reader.
    m_blocking = true;
    const QDeadlineTimer deadline(msecs < 0 ? QDeadlineTimer(QDeadlineTimer::Forever) : QDeadlineTimer(msecs));
    std::optional<Frame> frame;
    while (!m_failed) {
        if (!takeFrame(&frame)) {
            break;
        }
        if (frame) {
            break;
        }
        if (m_socket->state() != QLocalSocket::ConnectedState
            || !m_socket->waitForReadyRead(deadline.isForever() ? -1 : int(deadline.remainingTime()))) {
            break;
        }
        m_reader.feed(m_socket->readAll());
    }
    m_blocking = false;
    return frame;
}

std::optional<Connection::PeerCredentials> Connection::peerCredentials() const
{
    const qintptr fd = m_socket->socketDescriptor();
    if (fd == -1) {
        return std::nullopt;
    }
#if defined(Q_OS_LINUX)
    ucred credentials{};
    socklen_t length = sizeof(credentials);
    if (getsockopt(int(fd), SOL_SOCKET, SO_PEERCRED, &credentials, &length) != 0) {
        return std::nullopt;
    }
    return PeerCredentials{credentials.uid, credentials.gid, credentials.pid};
#else
    // Unverified on the BSDs: getpeereid reports no pid.
    uid_t uid;
    gid_t gid;
    if (getpeereid(int(fd), &uid, &gid) != 0) {
        return std::nullopt;
    }
    return PeerCredentials{uid, gid, -1};
#endif
}

QLocalSocket* Connection::socket() const
{
    return m_socket;
}

quint16 Connection::protocolVersion() const
{
    return m_protocolVersion;
}

void Connection::readAvailable()
{
    if (m_failed) {
        return;
    }
    m_reader.feed(m_socket->readAll());
    if (m_blocking) {
        return;
    }
    // A receiver may delete this connection while handling a frame.
    QPointer<Connection> self(this);
    for (;;) {
        std::optional<Frame> frame;
        if (!takeFrame(&frame) || !frame) {
            return;
        }
        Q_EMIT frameReceived(frame->type, frame->payload);
        if (!self || m_failed) {
            return;
        }
    }
}

bool Connection::takeFrame(std::optional<Frame>* frame)
{
    *frame = m_reader.next();
    if (!*frame) {
        if (m_reader.error() != FrameReader::Error::None) {
            handleReaderError();
            return false;
        }
        return true;
    }
    if ((*frame)->type == Frame::VersionRejected) {
        const QByteArray& payload = (*frame)->payload;
        const quint16 peerVersion = payload.size() >= 2 ? qFromBigEndian<quint16>(payload.constData()) : 0;
        sonicWarning() << "the peer rejected protocol version" << m_protocolVersion << "; it speaks version" << peerVersion;
        m_failed = true;
        frame->reset();
        Q_EMIT versionRejected(peerVersion);
        m_socket->abort();
        return false;
    }
    sonicDebug() << "received frame type" << (*frame)->type << "with" << (*frame)->payload.size() << "bytes";
    return true;
}

void Connection::handleReaderError()
{
    m_failed = true;
    QString reason = m_reader.errorString();
    if (m_reader.error() == FrameReader::Error::ProtocolMismatch) {
        // A real Sonic IPC peer with another version: tell it before closing, best effort.
        reason = QStringLiteral("protocol version mismatch: the peer speaks %1, this side %2").arg(m_reader.peerProtocolVersion()).arg(m_protocolVersion);
        m_socket->write(Internal::encodeVersionRejected(m_protocolVersion));
        m_socket->waitForBytesWritten(100);
    }
    sonicWarning() << "IPC protocol error:" << reason;
    Q_EMIT protocolError(reason);
    m_socket->abort();
}
}
