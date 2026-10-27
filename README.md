<!--
  - SPDX-FileCopyrightText: None
  - SPDX-License-Identifier: CC0-1.0
-->

# Sonic IPC

Sonic IPC is a small inter-process communication library for SonicDE. It is built on Unix domain sockets (`QLocalServer` / `QLocalSocket`) and `QDataStream`. Components can talk to each other directly, peer to peer, without a message bus, so D-Bus becomes an option rather than a requirement.

See [Sonic-DE/sonic#14](https://github.com/Sonic-DE/sonic/issues/14) for the background.

## Features

- **Framed messages:** each frame has a magic marker, a frame format version, the application's protocol version, a message type, a payload length and a CRC-32 of the payload. Damaged, oversized or foreign data is rejected and the connection is closed.
- **Protocol versions:** a peer speaking another protocol version is told so with a "version rejected" frame before the connection closes. The client side reports it through `versionRejected()` and stops reconnecting.
- **Packet-size limits:** the payload length is checked from the header alone, before any payload is buffered.
- **Asynchronous:** works through Qt's event loop (`frameReceived`, `disconnected`). Threads without an event loop use `Connection::waitForFrame()` / `sendBlocking()` or the blocking `Client::request()`.
- **Reconnection:** `Client::setReconnectInterval()` follows a server across restarts.
- **Socket permissions:** owner-only, group or world access. A live server's socket is never replaced: `listen()` fails while another server answers on the name, and only a stale socket file is removed.
- **Per-user socket paths:** `Server::userSocketPath()` gives `<runtime directory>/<name>`, or a checked `/tmp/sonic-<uid>/` fallback.
- **Peer credentials:** the uid, gid and pid of the connected process.
- **Broadcast to all connected clients.**

## Usage

```cmake
find_package(SonicIpc REQUIRED)
target_link_libraries(myapp PRIVATE Sonic::Ipc)
```

```cpp
#include <Sonic/Ipc/Connection>
#include <Sonic/Ipc/Server>

constexpr quint16 protocolVersion = 1;

auto *server = new Sonic::Ipc::Server(this);
// The name or path is passed to QLocalServer::listen() unchanged.
server->listen(Sonic::Ipc::Server::userSocketPath(QStringLiteral("my-service.sock")), Sonic::Ipc::Server::Access::User, protocolVersion);
connect(server, &Sonic::Ipc::Server::newConnection, this, [](Sonic::Ipc::Connection *connection) {
    QObject::connect(connection, &Sonic::Ipc::Connection::frameReceived, connection, [connection](quint32 type, const QByteArray &payload) {
        connection->send(type, payload); // echo
    });
});
```

```cpp
#include <Sonic/Ipc/Client>

// On a worker thread: one request, one reply, within 5 s.
QString error;
const std::optional<Sonic::Ipc::Frame> reply =
    Sonic::Ipc::Client::request(path, /*type*/ 1, payload, protocolVersion, 5000, &error);
```

Frame type `0xFFFFFFFF` is reserved for the library.

## Building

Sonic IPC is built as part of SonicDE with the [sonicde-builder](https://github.com/Sonic-DE/sonicde-builder):

```shell
./build-repo sonic-frameworks-ipc
```

It needs Qt 6.11 or newer (Core and Network), Extra CMake Modules and [Sonic Logging](https://github.com/Sonic-DE/sonic-frameworks-logging).
