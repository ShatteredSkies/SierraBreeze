set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(sierrabreeze_config_SRCS
    breezeconfigwidget.cpp
    breezedetectwidget.cpp
    breezeexceptiondialog.cpp
    breezeexceptionlistwidget.cpp
    breezeexceptionmodel.cpp
    breezeitemmodel.cpp
    ../breezeexceptionlist.cpp
)

set(sierrabreeze_config_PART_FORMS
    ui/breezeconfigurationui.ui
    ui/breezedetectwidget.ui
    ui/breezeexceptiondialog.ui
    ui/breezeexceptionlistwidget.ui
)

ki18n_wrap_ui(sierrabreeze_config_PART_FORMS_HEADERS ${sierrabreeze_config_PART_FORMS})

kcoreaddons_add_plugin(kcm_sierrabreezedecoration
    SOURCES
    kcm_sierrabreezedecoration.cpp
    ${sierrabreeze_config_SRCS}
    ${sierrabreeze_config_PART_FORMS_HEADERS}
    INSTALL_NAMESPACE "org.kde.kdecoration2.kcm")

kconfig_add_kcfg_files(kcm_sierrabreezedecoration ../breezesettings.kcfgc)
target_include_directories(kcm_sierrabreezedecoration PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
target_link_libraries(kcm_sierrabreezedecoration
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::DBus
    PRIVATE
        KF6::ConfigCore
        KF6::CoreAddons
        KF6::GuiAddons
        KF6::I18n
        KF6::KCMUtils
)
kcmutils_generate_desktop_file(kcm_sierrabreezedecoration)
