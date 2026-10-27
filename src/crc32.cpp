// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "crc32.h"

#include <array>

namespace Sonic::Ipc::Crc32 {
namespace {
    constexpr std::array<quint32, 256> makeTable()
    {
        std::array<quint32, 256> table{};
        for (quint32 i = 0; i < 256; ++i) {
            quint32 value = i;
            for (int bit = 0; bit < 8; ++bit) {
                value = (value & 1) ? (value >> 1) ^ 0xEDB88320u : value >> 1;
            }
            table[i] = value;
        }
        return table;
    }

    constexpr std::array<quint32, 256> table = makeTable();
}

quint32 compute(QByteArrayView data)
{
    quint32 crc = 0xFFFFFFFFu;
    for (const char c : data) {
        crc = table[(crc ^ static_cast<quint8>(c)) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}
}
