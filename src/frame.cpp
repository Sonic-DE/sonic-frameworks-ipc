// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "frame.h"
#include "crc32.h"
#include "ipc_p.h"

#include <Sonic/Logging/Log>

#include <QtEndian>

SONIC_LOGGING_COMPONENT("org.sonicde.frameworks.ipc")

namespace Sonic::Ipc {
QByteArray encodeFrame(quint32 type, QByteArrayView payload, quint16 protocolVersion)
{
    if (type == Frame::VersionRejected) {
        sonicWarning() << "frame type" << Qt::hex << type << "is reserved for the library; not sending it";
        return {};
    }
    return Internal::encodeFrameUnchecked(type, payload, protocolVersion);
}

namespace Internal {
    QByteArray encodeFrameUnchecked(quint32 type, QByteArrayView payload, quint16 protocolVersion)
    {
        QByteArray frame(frameHeaderSize, Qt::Uninitialized);
        auto* header = reinterpret_cast<uchar*>(frame.data());
        qToBigEndian<quint32>(frameMagic, header);
        qToBigEndian<quint16>(frameFormatVersion, header + 4);
        qToBigEndian<quint16>(protocolVersion, header + 6);
        qToBigEndian<quint32>(type, header + 8);
        qToBigEndian<quint32>(static_cast<quint32>(payload.size()), header + 12);
        qToBigEndian<quint32>(Crc32::compute(payload), header + 16);
        frame.append(payload);
        return frame;
    }

    QByteArray encodeVersionRejected(quint16 ownVersion)
    {
        uchar version[2];
        qToBigEndian<quint16>(ownVersion, version);
        return encodeFrameUnchecked(Frame::VersionRejected, QByteArrayView(reinterpret_cast<const char*>(version), 2), ownVersion);
    }
}
}
