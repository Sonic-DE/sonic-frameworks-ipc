<!--
  - SPDX-FileCopyrightText: None
  - SPDX-License-Identifier: CC0-1.0
-->

# Sonic IPC

Sonic IPC is a small inter-process communication library for SonicDE. It is built on Unix domain sockets (`QLocalServer` / `QLocalSocket`) and `QDataStream`. Components can talk to each other directly, peer to peer, without a message bus, so D-Bus becomes an option rather than a requirement.

> **Status:** in development. The API below is the planned design and may still change. See [Sonic-DE/sonic#14](https://github.com/Sonic-DE/sonic/issues/14).

## Features

- **Framed messages:** each frame has a magic marker, a protocol version, a message type, a payload length and a checksum. Damaged, oversized or foreign data is rejected and the connection is closed.
- **Packet-size limits:** these stop a peer from making the other side buffer unbounded data.
- **Asynchronous:** works through Qt's event loop (`frameReceived`, `disconnected`). A blocking request/response call is also available for helper processes.
- **Socket permissions:**
  - per-user sockets live in the user's runtime directory with mode 0600;
  - group or world access is available for system daemons that authenticate their peers themselves.
- **Peer credentials:** the uid, gid and pid of the connected process.
- **Broadcast to all connected clients.**

## Usage

```cmake
find_package(SonicIpc REQUIRED)
target_link_libraries(myapp PRIVATE Sonic::Ipc)
```

```cpp
#include <Sonic/Ipc/Server>

auto *server = new Sonic::Ipc::Server(this);
server->listen(QStringLiteral("my-service"), Sonic::Ipc::Server::Access::User);
connect(server, &Sonic::Ipc::Server::newConnection, this, [](Sonic::Ipc::Connection *connection) {
    QObject::connect(connection, &Sonic::Ipc::Connection::frameReceived, connection, [connection](quint32 type, const QByteArray &payload) {
        connection->send(type, payload); // echo
    });
});
```

## Building

Sonic IPC is built as part of SonicDE with the [sonicde-builder](https://github.com/Sonic-DE/sonicde-builder):

```shell
./build-repo sonic-frameworks-ipc
```

It needs Qt 6.11 or newer (Core and Network) and Extra CMake Modules.
