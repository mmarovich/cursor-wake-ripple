# Settings UI shown from System Settings → Desktop Effects → Cursor Wake Ripple → Configure
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS KCMUtils I18n ConfigWidgets)

kcoreaddons_add_plugin(kwin_cursorwakeripple_config
    SOURCES cursorwakeripple_config.cpp
    INSTALL_NAMESPACE "kwin/effects/configs"
)

target_link_libraries(kwin_cursorwakeripple_config
    KF6::KCMUtils
    KF6::I18n
    KF6::ConfigCore
    KF6::ConfigWidgets
    Qt6::DBus
    Qt6::Widgets
)

target_compile_definitions(kwin_cursorwakeripple_config PRIVATE TRANSLATION_DOMAIN=\"kwin_cursorwakeripple_config\")
