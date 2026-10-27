// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "sonicipc_export.h"

#include <QByteArray>
#include <QByteArrayView>

/*!
 * The Sonic IPC frame. Big-endian header of 20 bytes, then the payload:
 *
 * | quint32 magic 0x534F4E43 | quint16 frame format 1 | quint16 application protocol |
 * | quint32 type | quint32 payload length | quint32 CRC-32 of the payload |
 */
namespace Sonic::Ipc {
constexpr quint32 frameMagic = 0x534F4E43;
constexpr quint16 frameFormatVersion = 1;
constexpr qsizetype frameHeaderSize = 20;
constexpr qint64 defaultMaxPayload = 16 * 1024 * 1024;

struct Frame {
    //! Reserved for the library: tells a peer its protocol version was rejected. The payload is the sender's quint16 version.
    static constexpr quint32 VersionRejected = 0xFFFFFFFF;

    quint32 type = 0;
    QByteArray payload;
};

//! The frame bytes; empty, with a warning, for the reserved VersionRejected type.
SONICIPC_EXPORT QByteArray encodeFrame(quint32 type, QByteArrayView payload, quint16 protocolVersion);
}
