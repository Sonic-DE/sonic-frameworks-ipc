// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "sonicipc_export.h"

#include <QByteArray>
#include <QString>

// Not installed: library internals and hooks for the autotests.
namespace Sonic::Ipc::Internal {
//! Encodes any type, including the reserved VersionRejected.
QByteArray encodeFrameUnchecked(quint32 type, QByteArrayView payload, quint16 protocolVersion);
QByteArray encodeVersionRejected(quint16 ownVersion);

//! Server::userSocketPath() then sees an empty runtime location and uses this directory in place of /tmp.
SONICIPC_EXPORT void setUserSocketFallbackForTesting(bool emptyRuntimeLocation, const QString& temporaryDirectory);
}
