// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "frame.h"
#include "crc32.h"
#include "framereader.h"

#include <QRegularExpression>
#include <QTest>
#include <QtEndian>

using namespace Sonic::Ipc;

namespace {
// Rewrites one big-endian header field of an encoded frame.
template <typename T>
QByteArray patched(QByteArray frame, int offset, T value)
{
    qToBigEndian<T>(value, reinterpret_cast<uchar*>(frame.data()) + offset);
    return frame;
}
}

class FrameTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void crcVector()
    {
        QCOMPARE(Crc32::compute("123456789"), quint32(0xCBF43926));
    }

    void roundTrip()
    {
        const QByteArray frame = encodeFrame(7, "payload", 3);
        QCOMPARE(frame.size(), frameHeaderSize + 7);
        FrameReader reader(3);
        reader.feed(frame);
        const std::optional<Frame> decoded = reader.next();
        QVERIFY(decoded);
        QCOMPARE(decoded->type, quint32(7));
        QCOMPARE(decoded->payload, QByteArray("payload"));
        QVERIFY(!reader.next());
        QCOMPARE(reader.error(), FrameReader::Error::None);
    }

    void oneByteAtATime()
    {
        const QByteArray frame = encodeFrame(9, "split into single bytes", 1);
        FrameReader reader(1);
        std::optional<Frame> decoded;
        for (qsizetype i = 0; i < frame.size(); ++i) {
            QVERIFY(!decoded);
            reader.feed(frame.mid(i, 1));
            decoded = reader.next();
        }
        QVERIFY(decoded);
        QCOMPARE(decoded->payload, QByteArray("split into single bytes"));
    }

    void twoFramesInOneChunk()
    {
        FrameReader reader(1);
        reader.feed(QByteArray(encodeFrame(1, "first", 1) + encodeFrame(2, "second", 1)));
        const std::optional<Frame> first = reader.next();
        const std::optional<Frame> second = reader.next();
        QVERIFY(first && second);
        QCOMPARE(first->payload, QByteArray("first"));
        QCOMPARE(second->type, quint32(2));
        QCOMPARE(second->payload, QByteArray("second"));
        QVERIFY(!reader.next());
    }

    void zeroLengthPayload()
    {
        FrameReader reader(1);
        reader.feed(encodeFrame(4, QByteArrayView(), 1));
        const std::optional<Frame> decoded = reader.next();
        QVERIFY(decoded);
        QCOMPARE(decoded->type, quint32(4));
        QVERIFY(decoded->payload.isEmpty());
    }

    void errors_data()
    {
        QTest::addColumn<QByteArray>("bytes");
        QTest::addColumn<int>("error");
        const QByteArray good = encodeFrame(5, "content", 1);
        QTest::newRow("bad magic") << patched<quint32>(good, 0, 0xDEADBEEF) << int(FrameReader::Error::BadMagic);
        QTest::newRow("format version") << patched<quint16>(good, 4, 2) << int(FrameReader::Error::BadFormatVersion);
        QTest::newRow("protocol version") << encodeFrame(5, "content", 2) << int(FrameReader::Error::ProtocolMismatch);
        // The header alone claims too much; no payload follows.
        QTest::newRow("too large") << patched<quint32>(good, 12, 1024 * 1024).left(frameHeaderSize) << int(FrameReader::Error::TooLarge);
        QTest::newRow("bad checksum") << patched<quint32>(good, 16, 0x12345678) << int(FrameReader::Error::BadChecksum);
    }

    void errors()
    {
        QFETCH(QByteArray, bytes);
        QFETCH(int, error);
        FrameReader reader(1, 1024);
        reader.feed(bytes);
        QVERIFY(!reader.next());
        QCOMPARE(int(reader.error()), error);
        QVERIFY(!reader.errorString().isEmpty());
        // It stays in error.
        reader.feed(encodeFrame(1, "later", 1));
        QVERIFY(!reader.next());
        QCOMPARE(int(reader.error()), error);
    }

    void versionRejectedIsReservedAndReadableAcrossVersions()
    {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("reserved")));
        QVERIFY(encodeFrame(Frame::VersionRejected, "x", 1).isEmpty());

        // A rejection notice sent by a version-2 peer reaches a version-1 reader.
        QByteArray notice = encodeFrame(1, QByteArray(2, '\0'), 2);
        notice = patched<quint32>(notice, 8, Frame::VersionRejected);
        FrameReader reader(1);
        reader.feed(notice);
        const std::optional<Frame> decoded = reader.next();
        QVERIFY(decoded);
        QCOMPARE(decoded->type, Frame::VersionRejected);
    }
};

QTEST_GUILESS_MAIN(FrameTest)

#include "frametest.moc"
