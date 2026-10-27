// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "framereader.h"
#include "crc32.h"

#include <QtEndian>

namespace Sonic::Ipc {
FrameReader::FrameReader(quint16 expectedProtocol, qint64 maxPayload)
    : m_expectedProtocol(expectedProtocol)
    , m_maxPayload(maxPayload)
{
}

void FrameReader::feed(QByteArrayView data)
{
    if (m_error != Error::None) {
        return;
    }
    // Drop consumed bytes once they make up most of the buffer.
    if (m_offset > 0 && m_offset * 2 >= m_buffer.size()) {
        m_buffer.remove(0, m_offset);
        m_offset = 0;
    }
    m_buffer.append(data);
}

std::optional<Frame> FrameReader::next()
{
    if (m_error != Error::None || m_buffer.size() - m_offset < frameHeaderSize) {
        return std::nullopt;
    }
    const auto* header = reinterpret_cast<const uchar*>(m_buffer.constData() + m_offset);
    const quint32 magic = qFromBigEndian<quint32>(header);
    const quint16 format = qFromBigEndian<quint16>(header + 4);
    const quint16 protocol = qFromBigEndian<quint16>(header + 6);
    const quint32 type = qFromBigEndian<quint32>(header + 8);
    const quint32 length = qFromBigEndian<quint32>(header + 12);
    const quint32 checksum = qFromBigEndian<quint32>(header + 16);

    if (magic != frameMagic) {
        fail(Error::BadMagic, QStringLiteral("bad frame magic 0x%1").arg(magic, 8, 16, QLatin1Char('0')));
        return std::nullopt;
    }
    if (format != frameFormatVersion) {
        fail(Error::BadFormatVersion, QStringLiteral("unsupported frame format %1").arg(format));
        return std::nullopt;
    }
    m_peerProtocol = protocol;
    if (static_cast<qint64>(length) > m_maxPayload) {
        fail(Error::TooLarge, QStringLiteral("frame payload of %1 bytes exceeds the limit of %2").arg(length).arg(m_maxPayload));
        return std::nullopt;
    }
    // The rejection notice is readable whatever version its sender speaks.
    if (protocol != m_expectedProtocol && type != Frame::VersionRejected) {
        fail(Error::ProtocolMismatch, QStringLiteral("peer speaks protocol %1, this side %2").arg(protocol).arg(m_expectedProtocol));
        return std::nullopt;
    }
    if (m_buffer.size() - m_offset - frameHeaderSize < static_cast<qsizetype>(length)) {
        return std::nullopt;
    }
    const QByteArray payload = m_buffer.mid(m_offset + frameHeaderSize, length);
    if (Crc32::compute(payload) != checksum) {
        fail(Error::BadChecksum, QStringLiteral("frame checksum mismatch"));
        return std::nullopt;
    }
    m_offset += frameHeaderSize + length;
    return Frame{type, payload};
}

FrameReader::Error FrameReader::error() const
{
    return m_error;
}

QString FrameReader::errorString() const
{
    return m_errorString;
}

quint16 FrameReader::peerProtocolVersion() const
{
    return m_peerProtocol;
}

bool FrameReader::fail(Error error, const QString& text)
{
    m_error = error;
    m_errorString = text;
    m_buffer.clear();
    m_offset = 0;
    return false;
}
}
