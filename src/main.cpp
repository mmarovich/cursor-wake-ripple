/*
    SPDX-FileCopyrightText: 2026 Max Marovich
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "cursorwakeripple.h"

namespace KWin
{

KWIN_EFFECT_FACTORY_SUPPORTED(CursorWakeRippleEffect,
                              "metadata.json",
                              return CursorWakeRippleEffect::supported();)

} // namespace KWin

#include "main.moc"
