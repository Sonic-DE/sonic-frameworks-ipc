// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "sonicipc_export.h"

#include <QByteArray>
#include <QByteArrayView>

/*!
 * \namespace Sonic::Ipc
 * \inmodule SonicIpc
 * \inheaderfile Sonic/Ipc/Frame
 *
 * \brief Framed peer-to-peer messages over local sockets.
 *
 * The Sonic IPC frame. Big-endian header of 20 bytes, then the payload:
 *
 * \table
 * \header \li Type \li Field
 * \row \li quint32 \li magic 0x534F4E43
 * \row \li quint16 \li frame format 1
 * \row \li quint16 \li application protocol
 * \row \li quint32 \li type
 * \row \li quint32 \li payload length
 * \row \li quint32 \li CRC-32 of the payload
 * \endtable
 */
namespace Sonic::Ipc {
/*!
 * \variable Sonic::Ipc::frameMagic
 * The first four bytes of every frame, "SONC".
 */
constexpr quint32 frameMagic = 0x534F4E43;
/*!
 * \variable Sonic::Ipc::frameFormatVersion
 * The frame format this library writes and reads.
 */
constexpr quint16 frameFormatVersion = 1;
/*!
 * \variable Sonic::Ipc::frameHeaderSize
 * The header's size in bytes.
 */
constexpr qsizetype frameHeaderSize = 20;
/*!
 * \variable Sonic::Ipc::defaultMaxPayload
 * The largest payload accepted unless a caller sets another limit: 16 MiB.
 */
constexpr qint64 defaultMaxPayload = 16 * 1024 * 1024;

/*!
 * \class Sonic::Ipc::Frame
 * \inmodule SonicIpc
 * \inheaderfile Sonic/Ipc/Frame
 *
 * \brief One message: the application's type and its payload.
 */
struct Frame {
    /*!
     * \variable Sonic::Ipc::Frame::VersionRejected
     * Reserved for the library: tells a peer its protocol version was rejected. The payload is the sender's quint16 version.
     */
    static constexpr quint32 VersionRejected = 0xFFFFFFFF;

    /*!
     * \variable Sonic::Ipc::Frame::type
     * The application's message type.
     */
    quint32 type = 0;
    /*!
     * \variable Sonic::Ipc::Frame::payload
     * The message's bytes.
     */
    QByteArray payload;
};

/*!
 * \fn QByteArray Sonic::Ipc::encodeFrame(quint32 type, QByteArrayView payload, quint16 protocolVersion)
 *
 * The frame bytes for \a payload of \a type in application protocol
 * \a protocolVersion; empty, with a warning, for the reserved
 * Frame::VersionRejected type.
 */
SONICIPC_EXPORT QByteArray encodeFrame(quint32 type, QByteArrayView payload, quint16 protocolVersion);
}
