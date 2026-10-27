// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "frame.h"
#include "sonicipc_export.h"

#include <QByteArray>
#include <QString>

#include <optional>

namespace Sonic::Ipc {
/*!
 * Splits a byte stream into frames. The payload length is checked as soon as
 * the header is complete, before any payload is buffered. After an error the
 * reader stays in error; the caller closes the connection.
 */
class SONICIPC_EXPORT FrameReader {
public:
    enum class Error {
        None,
        BadMagic,
        BadFormatVersion,
        ProtocolMismatch,
        TooLarge,
        BadChecksum,
    };

    explicit FrameReader(quint16 expectedProtocol, qint64 maxPayload = defaultMaxPayload);

    void feed(QByteArrayView data);
    std::optional<Frame> next();

    Error error() const;
    QString errorString() const;
    //! The protocol version in the last header read; names the peer's version after ProtocolMismatch.
    quint16 peerProtocolVersion() const;

private:
    bool fail(Error error, const QString& text);

    quint16 m_expectedProtocol;
    qint64 m_maxPayload;
    QByteArray m_buffer;
    qsizetype m_offset = 0;
    Error m_error = Error::None;
    QString m_errorString;
    quint16 m_peerProtocol = 0;
};
}
