#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Fire one ripple at the current cursor position (run inside the session that
# has the effect loaded; for nested testing, run it from the nested konsole).
exec dbus-send --session --type=method_call --dest=org.kde.KWin \
    /CursorWakeRipple org.kde.kwin.CursorWakeRipple.trigger
