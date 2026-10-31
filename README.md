# Cursor Wake Ripple

A KWin 6 desktop effect for KDE Plasma 6. After the mouse has been idle for a
while, the next movement sends a **ripple through the screen under the cursor**,
so you can find the pointer again right away.

It's a real screen-space OpenGL shader, not an overlay. The composited desktop
is redrawn with displaced texture coordinates, so windows, wallpaper and panels
bend under the wave and nothing gets covered up. When no ripple is running, the
effect is a pass-through and costs nothing.

- Triggers on the first pointer movement after a configurable idle time
- Ripple size can be set from System Settings
- Only the monitor with the cursor is affected
- Can also be fired on demand over D-Bus (for example from a keyboard shortcut)

## Requirements

- KDE Plasma 6 with **KWin 6** (developed and tested on KWin 6.3.6, Wayland session)
- An OpenGL compositing backend (the default on Plasma 6)
- Build dependencies that match your **exact** KWin version

> KWin effect plugins have no stable ABI. The plugin is version-stamped against the
> KWin it was built with, so **rebuild after every Plasma/KWin upgrade**. A mismatched
> build just won't load (it won't crash KWin).
> Check your version with `kwin_wayland --version`.

### Debian 13 (trixie) / Kubuntu / KDE neon packages

```bash
sudo apt install build-essential cmake extra-cmake-modules kwin-dev \
  qt6-base-dev qt6-declarative-dev \
  libkf6config-dev libkf6coreaddons-dev libkf6windowsystem-dev \
  libkf6kcmutils-dev libkf6i18n-dev libkf6configwidgets-dev \
  libepoxy-dev libwayland-dev libxkbcommon-dev
# optional, to validate the shaders offline:
sudo apt install glslang-tools
```

On other distros, install the KWin development headers plus Qt 6 (Core, Gui, DBus,
Widgets), KF6 Config/CoreAddons/KCMUtils/I18n/ConfigWidgets, ECM and libepoxy.

## Build

```bash
git clone https://github.com/mmarovich/cursor-wake-ripple.git
cd cursor-wake-ripple
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j"$(nproc)"
```

## Try it safely first (nested KWin)

A crashing effect takes down `kwin_wayland`, and on Wayland that ends your whole session.
So test the fresh build inside a nested KWin window first. No install and no sudo needed:

```bash
./scripts/nested-test.sh                 # 1600x900 window running its own KWin + Konsole
IDLE_MS=3000 ./scripts/nested-test.sh    # custom idle time for the test
```

Leave the mouse still inside that window for about a second, then move it. You can
also run `./scripts/trigger.sh` from the nested Konsole to fire a ripple. The script uses
a throwaway `XDG_CONFIG_HOME` and a private D-Bus session, so your real `kwinrc` isn't touched.

## Install

```bash
sudo cmake --install build
```

This installs the effect and its settings page to the system Qt plugin directory
(on Debian: `/usr/lib/x86_64-linux-gnu/qt6/plugins/kwin/effects/`).

## Enable

**System Settings → Window Management → Desktop Effects** → search for "ripple"
(it's listed under *Accessibility*) → tick **Cursor Wake Ripple** → **Apply**.

Or from a terminal:

```bash
kwriteconfig6 --file kwinrc --group Plugins --key cursorwakerippleEnabled true
dbus-send --session --print-reply --dest=org.kde.KWin /Effects \
  org.kde.kwin.Effects.loadEffect string:cursorwakeripple
```

To turn it off, untick it in Desktop Effects, or run:

```bash
dbus-send --session --print-reply --dest=org.kde.KWin /Effects \
  org.kde.kwin.Effects.unloadEffect string:cursorwakeripple
```

Note: once KWin has loaded the plugin, it won't pick up a rebuilt `.so`. After you
reinstall, log out and back in, or use the nested test.

## Settings

Click the gear icon next to Cursor Wake Ripple in Desktop Effects:

| Setting | Range | Default | Meaning |
|---|---|---|---|
| Idle time before trigger | 1–600 s | 30 s ("Defaults" button) | How long the pointer must stay still before the next move ripples |
| Ripple size | 0.05–0.60 | 0.18 | Final ring radius, as a fraction of screen height |

Saving reloads the effect right away.

### Advanced (`~/.config/kwinrc`)

There are more options without a GUI yet. They live in the same group:

```ini
[Effect-cursorwakeripple]
IdleTimeoutMs=30000   # idle time in ms; 0 = only trigger over D-Bus
MaxRadius=0.18        # ripple size (fraction of screen height)
DurationMs=1000       # how long the ripple runs (100–5000)
Amplitude=0.04        # max displacement (fraction of screen height)
Wavelength=0.035      # wave spacing
RingWidth=0.05        # width of the wave packet
FlipY=false           # see below
```

Apply changes without logging out:

```bash
dbus-send --session --print-reply --dest=org.kde.KWin /Effects \
  org.kde.kwin.Effects.reconfigureEffect string:cursorwakeripple
```

**FlipY:** framebuffer texture orientation can differ between drivers and setups. If the
ripple is centred at a vertically mirrored spot (not under your cursor), set `FlipY=true`.

## D-Bus / keyboard shortcut

The effect exports `org.kde.KWin /CursorWakeRipple org.kde.kwin.CursorWakeRipple` with
`trigger()` (at the current cursor position) and `triggerAt(double x, double y)`.

To bind a "find my cursor" key: System Settings → Keyboard → Shortcuts → Add New →
Command → `/path/to/cursor-wake-ripple/scripts/trigger.sh`.

## How it works

1. On `EffectsHandler::mouseChanged`, if the pointer has been idle for at least
   `IdleTimeoutMs`, a ripple starts at the cursor.
2. While it runs, `paintScreen()` renders the whole scene for that output into an
   offscreen `GLFramebuffer` (the same approach KWin's own Zoom effect uses).
3. That texture is drawn back through `src/shaders/ripple*.frag`. An outward-moving
   Gaussian wave packet shifts each pixel's sample point along the radius, with an
   amplitude that fades as `(1 - progress)^2`.
4. When the ripple ends, the offscreen texture is freed and the effect goes back to
   being a pass-through.

**Why a C++ plugin and not a KWin script?** Scripted effects can only shade one window's
own texture. They can't read the composited framebuffer, so they can't make a single
ripple flow across window edges, panels and the wallpaper.

## Known limitations / roadmap

- Built and tested against KWin 6.3. KWin 6.5+ changed the `paintScreen`/`prePaintScreen`
  signatures, so it will need a port for newer versions. PRs welcome.
- No GUI yet for duration/speed/amplitude (they're in `kwinrc`, see above)
- Doesn't skip fullscreen games/video yet, and doesn't follow the global "animation speed" setting
- Screenshots/GIF, packaging (AUR, store.kde.org)

## License

GPL-2.0-or-later, the same as KWin. See [`LICENSES/GPL-2.0-or-later.txt`](LICENSES/GPL-2.0-or-later.txt).
Source files carry SPDX headers.

## Credits

Created by **Max Marovich**, 2026.
