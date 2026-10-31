# SPDX-License-Identifier: GPL-2.0-or-later
# Installs to ${KDE_INSTALL_PLUGINDIR}/kwin/effects/plugins/cursorwakeripple.so
# (Debian system prefix: /usr/lib/x86_64-linux-gnu/qt6/plugins/kwin/effects/plugins/)
kcoreaddons_add_plugin(cursorwakeripple INSTALL_NAMESPACE "kwin/effects/plugins")

target_sources(cursorwakeripple PRIVATE
    main.cpp
    cursorwakeripple.cpp
    cursorwakeripple.qrc
)

target_link_libraries(cursorwakeripple PRIVATE
    KWin::kwin
    Qt6::Core
    Qt6::Gui
    Qt6::DBus
    KF6::ConfigCore
    KF6::CoreAddons
    epoxy::epoxy
)

add_subdirectory(config)
