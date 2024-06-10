cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(breezesierra)
set(PROJECT_VERSION "0.1")
set(PROJECT_VERSION_MAJOR 0)

include(WriteBasicConfigVersionFile)
include(FeatureSummary)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(QT_MIN_VERSION "6.6.0")
find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Core Gui Widgets DBus)
find_package(ECM 0.0.9 REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_SOURCE_DIR}/cmake)
include(ECMInstallIcons)
include(KDEInstallDirs6)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(GenerateExportHeader)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(KF6_MIN_VERSION "5.27.0")
set(KDECORATION2_MIN_VERSION "5.27.80")

find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS CoreAddons GuiAddons ConfigWidgets WindowSystem I18n KCMUtils)
find_package(KDecoration2 ${KDECORATION2_MIN_VERSION} REQUIRED)

set(sierrabreeze_SRCS
    breezebutton.cpp
    breezedecoration.cpp
    breezeexceptionlist.cpp
    breezesettingsprovider.cpp
)

kconfig_add_kcfg_files(sierrabreeze_SRCS breezesettings.kcfgc)

add_library(sierrabreeze MODULE
    ${sierrabreeze_SRCS}
)

target_link_libraries(sierrabreeze
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::DBus
    PRIVATE
        KDecoration2::KDecoration
        KF6::ConfigCore
        KF6::CoreAddons
        KF6::ConfigWidgets
        KF6::GuiAddons
        KF6::I18n
        KF6::KCMUtils
        KF6::WindowSystem)

install(TARGETS sierrabreeze DESTINATION ${KDE_INSTALL_PLUGINDIR}/${KDECORATION_PLUGIN_DIR})

add_subdirectory(config)
