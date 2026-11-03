// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "sonicipc_export.h"

#include <QByteArrayView>

/*!
 * \namespace Sonic::Ipc::Crc32
 * \inmodule SonicIpc
 * \inheaderfile Sonic/Ipc/Crc32
 *
 * \brief The checksum of frame payloads.
 */
namespace Sonic::Ipc::Crc32 {
/*!
 * \fn quint32 Sonic::Ipc::Crc32::compute(QByteArrayView data)
 *
 * The CRC-32 of \a data as in IEEE 802.3: reflected, polynomial 0xEDB88320,
 * initial value and final XOR 0xFFFFFFFF. Qt's qChecksum only offers CRC-16.
 */
SONICIPC_EXPORT quint32 compute(QByteArrayView data);
}
