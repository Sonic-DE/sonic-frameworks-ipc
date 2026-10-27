# Graph Report - sonic-frameworks-ipc  (2026-10-27)

## Corpus Check
- 16 files · ~9,178 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 9 file(s) not represented in the graph (top: (none) 8, .in 1)

## Summary
- 193 nodes · 303 edges · 10 communities
- Extraction: 96% EXTRACTED · 4% INFERRED · 0% AMBIGUOUS · INFERRED: 11 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `34b6c700`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- ServerTest
- server.cpp
- client.h
- connection.cpp
- encodeFrame
- Frame
- client.cpp
- frametest.cpp
- crc32.cpp
- Sonic IPC

## God Nodes (most connected - your core abstractions)
1. `ServerTest` - 26 edges
2. `encodeFrame()` - 14 edges
3. `FrameTest` - 10 edges
4. `connectClient()` - 10 edges
5. `Frame` - 10 edges
6. `Client::request()` - 9 edges
7. `Server::listen()` - 8 edges
8. `modeOf()` - 6 edges
9. `encodeFrameUnchecked()` - 6 edges
10. `echoFrames()` - 5 edges

## Surprising Connections (you probably didn't know these)
- `Sonic IPC` --references--> `QLocalServer`  [INFERRED]
  README.md → src/server.h
- `connectClient()` --references--> `Connection()`  [EXTRACTED]
  autotests/servertest.cpp → src/connection.h
- `Client::request()` --calls--> `encodeFrame()`  [INFERRED]
  src/client.cpp → src/frame.cpp
- `Connection::send()` --calls--> `encodeFrame()`  [INFERRED]
  src/connection.cpp → src/frame.cpp
- `Client::connection()` --references--> `Connection()`  [EXTRACTED]
  src/client.cpp → src/connection.h

## Import Cycles
- None detected.

## Communities (10 total, 0 thin omitted)

### Community 0 - "ServerTest"
Cohesion: 0.07
Nodes (22): connectClient(), Q_OBJECT, QObject, QString, echoFrames(), modeOf(), ServerTest, m_counter (+14 more)

### Community 1 - "server.cpp"
Cohesion: 0.10
Nodes (25): Access, cerrno, qdir, qfile, qlocalserver, qstandardpaths, QByteArrayView, qint64 (+17 more)

### Community 2 - "client.h"
Cohesion: 0.14
Nodes (14): optional, qbytearrayview, QList, qobject, qstring, sonicipc_export, Client::connection(), Connection (+6 more)

### Community 3 - "connection.cpp"
Cohesion: 0.13
Nodes (18): PeerCredentials, QLocalSocket, qpointer, Connection::Connection(), Connection::peerCredentials(), Connection::protocolVersion(), Connection::send(), Connection::sendBlocking() (+10 more)

### Community 4 - "encodeFrame"
Cohesion: 0.18
Nodes (10): Q_OBJECT, QObject, FrameTest, QByteArray, QByteArrayView, quint16, quint32, encodeFrame() (+2 more)

### Community 5 - "Frame"
Cohesion: 0.12
Nodes (18): Error, Frame, payload, type, VersionRejected, quint32, optional, QByteArrayView (+10 more)

### Community 6 - "client.cpp"
Cohesion: 0.13
Nodes (13): log, qdeadlinetimer, qtimer, Client::Client(), Client::connectToServer(), Client::request(), optional, QByteArrayView (+5 more)

### Community 7 - "frametest.cpp"
Cohesion: 0.20
Nodes (9): QByteArray, patched(), crc32, frame, framereader, qregularexpression, qtendian, qtest (+1 more)

### Community 8 - "crc32.cpp"
Cohesion: 0.47
Nodes (5): array, compute(), QByteArrayView, quint32, makeTable()

### Community 9 - "Sonic IPC"
Cohesion: 0.33
Nodes (5): Building, Features, Sonic IPC, Usage, QLocalServer

## Knowledge Gaps
- **15 isolated node(s):** `m_dir`, `m_path`, `m_counter`, `QLocalSocket`, `Connection` (+10 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 72 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `encodeFrame()` connect `encodeFrame` to `connection.cpp`, `client.cpp`?**
  _High betweenness centrality (0.097) - this node is a cross-community bridge._
- **Why does `Client::request()` connect `client.cpp` to `encodeFrame`, `Frame`?**
  _High betweenness centrality (0.073) - this node is a cross-community bridge._
- **Are the 9 inferred relationships involving `encodeFrame()` (e.g. with `.errors()` and `.errors_data()`) actually correct?**
  _`encodeFrame()` has 9 INFERRED edges - model-reasoned connections that need verification._
- **What connects `m_dir`, `m_path`, `m_counter` to the rest of the system?**
  _15 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `ServerTest` be split into smaller, more focused modules?**
  _Cohesion score 0.06736353077816493 - nodes in this community are weakly interconnected._
- **Should `server.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.09523809523809523 - nodes in this community are weakly interconnected._
- **Should `client.h` be split into smaller, more focused modules?**
  _Cohesion score 0.13768115942028986 - nodes in this community are weakly interconnected._