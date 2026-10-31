#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Run a NESTED KWin (a window inside your normal Plasma session) with the
# freshly built effect. If the effect crashes, only this window dies - your
# real session is untouched. No sudo / install needed.
set -euo pipefail
HERE="$(cd "$(dirname "$0")/.." && pwd)"
PLUGIN_DIR="$HERE/build/bin"           # contains kwin/effects/plugins/cursorwakeripple.so
CFG="${TMPDIR:-/tmp}/cwr-nested-config"  # throwaway config, never touches ~/.config/kwinrc
IDLE_MS="${IDLE_MS:-1000}"

[ -f "$PLUGIN_DIR/kwin/effects/plugins/cursorwakeripple.so" ] || { echo "Build first: cmake --build build"; exit 1; }
mkdir -p "$CFG"
cat > "$CFG/kwinrc" <<CONF
[Plugins]
cursorwakerippleEnabled=true

[Effect-cursorwakeripple]
IdleTimeoutMs=$IDLE_MS
FlipY=false
Amplitude=0.05
CONF

export XDG_CONFIG_HOME="$CFG"
export QT_PLUGIN_PATH="$PLUGIN_DIR${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}"
export QT_LOGGING_RULES="kwin.effect.cursorwakeripple=true;kwin_core.debug=false"
# Private session bus so the nested KWin's org.kde.KWin name can't clash with the real one.
exec dbus-run-session -- kwin_wayland --width 1600 --height 900 --xwayland --exit-with-session=konsole
