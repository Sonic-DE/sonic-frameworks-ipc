# SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_library(SonicIpc)
add_library(Sonic::Ipc ALIAS SonicIpc)
set_target_properties(SonicIpc PROPERTIES
    VERSION     ${SONICIPC_VERSION}
    SOVERSION   ${SONICIPC_SOVERSION}
    EXPORT_NAME Ipc
)

target_sources(SonicIpc PRIVATE
    client.cpp
    client.h
    connection.cpp
    connection.h
    crc32.cpp
    crc32.h
    frame.cpp
    frame.h
    framereader.cpp
    framereader.h
    ipc_p.h
    server.cpp
    server.h
)

ecm_generate_export_header(SonicIpc
    BASE_NAME SonicIpc
    VERSION ${KF_VERSION}
    # Quoted, so the export header finds the version header installed beside it.
    USE_VERSION_HEADER "\"sonicipc_version.h\""
    VERSION_BASE_NAME SonicIpc
    DEPRECATED_BASE_VERSION 0
)

target_link_libraries(SonicIpc PUBLIC Qt6::Core Qt6::Network PRIVATE Sonic::Logging)

target_include_directories(SonicIpc
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR}>"
    INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}>"
)

install(TARGETS SonicIpc EXPORT SonicIpcTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

# <Sonic/Ipc/Name> forwards to the lowercase header installed next to it.
install(FILES
    Client
    Connection
    Crc32
    Frame
    FrameReader
    Server
    client.h
    connection.h
    crc32.h
    frame.h
    framereader.h
    server.h
    ${CMAKE_CURRENT_BINARY_DIR}/sonicipc_export.h
    DESTINATION ${SONICIPC_INSTALL_INCLUDEDIR}
    COMPONENT Devel
)
