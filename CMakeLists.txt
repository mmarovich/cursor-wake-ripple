# SPDX-FileCopyrightText: 2026 Max Marovich
# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.20)
project(cursor-wake-ripple VERSION 0.1.0 LANGUAGES CXX)

set(QT_MIN_VERSION "6.6.0")
set(KF6_MIN_VERSION "6.0.0")

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Core Gui DBus Widgets)  # Widgets: required by KWin::kwin link interface
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS Config CoreAddons)
# KWin's exported CMake package (Debian: kwin-dev). KWin effect plugins have NO
# stable ABI: rebuild after every Plasma/KWin upgrade.
find_package(KWin REQUIRED COMPONENTS kwin)
find_package(epoxy REQUIRED)

set(CMAKE_CXX_STANDARD 20)  # KWin 6 headers use C++20 (span, ranges, <=>)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Let AutoMoc recognise the KWin factory macros (they expand to a Q_OBJECT class).
list(APPEND CMAKE_AUTOMOC_MACRO_NAMES KWIN_EFFECT_FACTORY KWIN_EFFECT_FACTORY_SUPPORTED KWIN_EFFECT_FACTORY_ENABLED KWIN_EFFECT_FACTORY_SUPPORTED_ENABLED)

add_subdirectory(src)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
