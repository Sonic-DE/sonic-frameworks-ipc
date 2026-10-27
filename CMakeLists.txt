# SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.30)

set(KF_VERSION "6.31.0") # handled by release scripts
project(SonicIpc VERSION ${KF_VERSION})

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FeatureSummary)
find_package(ECM 6.31.0 NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://github.com/Sonic-DE/sonic-frameworks-cmake-modules")
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND FATAL_ON_MISSING_REQUIRED_PACKAGES)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(ECMGenerateExportHeader)
include(ECMSetupVersion)

include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(KDEGitCommitHooks)
include(CMakePackageConfigHelpers)

set(REQUIRED_QT_VERSION 6.11.0)
find_package(Qt6 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Core Network)
find_package(SonicLogging ${KF_VERSION} CONFIG REQUIRED)

set(sonicipc_version_header "${CMAKE_CURRENT_BINARY_DIR}/src/sonicipc_version.h")
ecm_setup_version(PROJECT VARIABLE_PREFIX SONICIPC
                        VERSION_HEADER "${sonicipc_version_header}"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/SonicIpcConfigVersion.cmake"
                        SOVERSION 6)

set(SONICIPC_INSTALL_INCLUDEDIR "${KDE_INSTALL_INCLUDEDIR}/Sonic/Ipc")

add_subdirectory(src)

if(BUILD_TESTING)
    find_package(Qt6 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test Concurrent)
    add_subdirectory(autotests)
endif()

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/SonicIpc")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/SonicIpcConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/SonicIpcConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/SonicIpcConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/SonicIpcConfigVersion.cmake"
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel)

install(EXPORT SonicIpcTargets DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        FILE SonicIpcTargets.cmake NAMESPACE Sonic:: COMPONENT Devel)

install(FILES ${sonicipc_version_header}
        DESTINATION ${SONICIPC_INSTALL_INCLUDEDIR} COMPONENT Devel)

include(ECMFeatureSummary)
ecm_feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
