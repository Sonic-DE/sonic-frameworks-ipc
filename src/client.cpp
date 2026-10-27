// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "client.h"
#include "connection.h"
#include "framereader.h"

#include <Sonic/Logging/Log>

#include <QDeadlineTimer>
#include <QLocalSocket>
#include <QTimer>
#include <QtEndian>

namespace Sonic::Ipc {
Client::Client(quint16 protocolVersion, qint64 maxPayload, QObject* parent)
    : QObject(parent)
    , m_protocolVersion(protocolVersion)
    , m_maxPayload(maxPayload)
    , m_reconnectTimer(new QTimer(this))
{
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &Client::attempt);
}

Client::~Client() = default;

void Client::connectToServer(const QString& nameOrPath)
{
    m_name = nameOrPath;
    m_stopped = false;
    attempt();
}

void Client::setReconnectInterval(int msecs)
{
    m_reconnectTimer->setInterval(msecs);
}

void Client::disconnectFromServer()
{
    m_stopped = true;
    m_reconnectTimer->stop();
    if (m_pendingSocket) {
        m_pendingSocket->abort();
        m_pendingSocket->deleteLater();
        m_pendingSocket = nullptr;
    }
    if (m_connection) {
        Connection* connection = m_connection;
        m_connection = nullptr;
        disconnect(connection, nullptr, this, nullptr);
        connection->socket()->abort();
        connection->deleteLater();
    }
}

Connection* Client::connection() const
{
    return m_connection;
}

void Client::attempt()
{
    if (m_stopped || m_pendingSocket || m_connection) {
        return;
    }
    auto* socket = new QLocalSocket(this);
    m_pendingSocket = socket;
    // Direct connections: QLocalSocket's signal argument types aren't registered metatypes.
    connect(
        socket,
        &QLocalSocket::connected,
        this,
        [this, socket] {
            disconnect(socket, nullptr, this, nullptr);
            m_pendingSocket = nullptr;
            m_outageReported = false;
            auto* connection = new Connection(socket, m_protocolVersion, m_maxPayload, this);
            m_connection = connection;
            connect(connection, &Connection::disconnected, this, [this, connection] {
                if (m_connection == connection) {
                    m_connection = nullptr;
                }
                connection->deleteLater();
                scheduleReconnect();
            });
            connect(connection, &Connection::versionRejected, this, [this](quint16 serverVersion) {
                // The same server would reject again: stop for good.
                m_stopped = true;
                m_reconnectTimer->stop();
                Q_EMIT versionRejected(serverVersion);
            });
            Q_EMIT connected(connection);
        },
        Qt::DirectConnection);
    connect(
        socket,
        &QLocalSocket::errorOccurred,
        this,
        [this, socket](QLocalSocket::LocalSocketError) {
            const QString reason = socket->errorString();
            disconnect(socket, nullptr, this, nullptr);
            m_pendingSocket = nullptr;
            socket->deleteLater();
            if (!m_outageReported) {
                m_outageReported = true;
                sonicWarning() << "cannot connect to" << m_name << ":" << reason;
            }
            Q_EMIT connectionFailed(reason);
            scheduleReconnect();
        },
        Qt::DirectConnection);
    socket->connectToServer(m_name);
}

void Client::scheduleReconnect()
{
    if (!m_stopped && m_reconnectTimer->interval() > 0) {
        m_reconnectTimer->start();
    }
}

std::optional<Frame>
Client::request(const QString& nameOrPath, quint32 type, QByteArrayView payload, quint16 protocolVersion, int timeoutMs, QString* error, qint64 maxPayload)
{
    const auto fail = [error](const QString& text) {
        if (error) {
            *error = text;
        }
        return std::optional<Frame>();
    };
    const QDeadlineTimer deadline(timeoutMs < 0 ? QDeadlineTimer(QDeadlineTimer::Forever) : QDeadlineTimer(timeoutMs));
    const auto remaining = [&deadline] {
        return deadline.isForever() ? -1 : int(deadline.remainingTime());
    };

    const QByteArray frame = encodeFrame(type, payload, protocolVersion);
    if (frame.isEmpty()) {
        return fail(QStringLiteral("frame type %1 is reserved").arg(type));
    }

    QLocalSocket socket;
    socket.connectToServer(nameOrPath);
    if (!socket.waitForConnected(remaining())) {
        return fail(QStringLiteral("cannot connect to %1: %2").arg(nameOrPath, socket.errorString()));
    }
    socket.write(frame);
    while (socket.bytesToWrite() > 0) {
        if (!socket.waitForBytesWritten(remaining())) {
            return fail(QStringLiteral("cannot send to %1: %2").arg(nameOrPath, socket.errorString()));
        }
    }

    FrameReader reader(protocolVersion, maxPayload);
    for (;;) {
        if (std::optional<Frame> reply = reader.next()) {
            if (reply->type == Frame::VersionRejected) {
                const quint16 serverVersion = reply->payload.size() >= 2 ? qFromBigEndian<quint16>(reply->payload.constData()) : 0;
                sonicWarning() << nameOrPath << "rejected protocol version" << protocolVersion << "; it speaks version" << serverVersion;
                return fail(QStringLiteral("the server speaks protocol %1, this side %2").arg(serverVersion).arg(protocolVersion));
            }
            sonicDebug() << "received reply type" << reply->type << "with" << reply->payload.size() << "bytes";
            return reply;
        }
        if (reader.error() != FrameReader::Error::None) {
            sonicWarning() << "IPC protocol error from" << nameOrPath << ":" << reader.errorString();
            return fail(reader.errorString());
        }
        if (!socket.waitForReadyRead(remaining())) {
            return fail(QStringLiteral("no reply from %1: %2").arg(nameOrPath, socket.errorString()));
        }
        reader.feed(socket.readAll());
    }
}
}
