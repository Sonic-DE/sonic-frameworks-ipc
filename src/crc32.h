// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "sonicipc_export.h"

#include <QByteArrayView>

namespace Sonic::Ipc::Crc32 {
//! CRC-32 as in IEEE 802.3: reflected, polynomial 0xEDB88320, initial value and final XOR 0xFFFFFFFF.
//! Qt's qChecksum only offers CRC-16.
SONICIPC_EXPORT quint32 compute(QByteArrayView data);
}
