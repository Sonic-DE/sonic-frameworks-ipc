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
 * \class Sonic::Ipc::FrameReader
 * \inmodule SonicIpc
 * \inheaderfile Sonic/Ipc/FrameReader
 *
 * \brief Splits a byte stream into frames.
 *
 * The payload length is checked as soon as
 * the header is complete, before any payload is buffered. After an error the
 * reader stays in error; the caller closes the connection.
 */
class SONICIPC_EXPORT FrameReader {
public:
    /*!
     * What the reader rejected.
     *
     * \value None No error.
     * \value BadMagic The data doesn't start with frameMagic: not a Sonic IPC peer.
     * \value BadFormatVersion An unsupported frame format.
     * \value ProtocolMismatch The peer speaks another application protocol version.
     * \value TooLarge The payload is larger than the limit.
     * \value BadChecksum The payload doesn't match its CRC-32.
     */
    enum class Error {
        None,
        BadMagic,
        BadFormatVersion,
        ProtocolMismatch,
        TooLarge,
        BadChecksum,
    };

    /*!
     * Creates a reader for application protocol \a expectedProtocol that
     * rejects payloads larger than \a maxPayload bytes.
     */
    explicit FrameReader(quint16 expectedProtocol, qint64 maxPayload = defaultMaxPayload);

    /*!
     * Appends \a data received from the stream.
     */
    void feed(QByteArrayView data);
    /*!
     * The next complete frame; nothing while a frame is incomplete or after an error.
     */
    std::optional<Frame> next();

    /*!
     * What the reader rejected, or Error::None.
     */
    Error error() const;
    /*!
     * A description of error().
     */
    QString errorString() const;
    /*!
     * The protocol version in the last header read; names the peer's version after ProtocolMismatch.
     */
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
