/*
    Cursor Wake Ripple - KWin 6 effect
    SPDX-FileCopyrightText: 2026 Max Marovich
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include "core/colorspace.h"
#include "effect/effect.h"

#include <QElapsedTimer>
#include <QPointF>
#include <chrono>
#include <map>
#include <memory>

namespace KWin
{

class GLFramebuffer;
class GLShader;
class GLTexture;

/**
 * Screen-space ripple under the cursor.
 *
 * Technique (same as KWin's zoom effect): while the ripple is running, the
 * output containing the cursor is first composited into an offscreen
 * GLTexture, then drawn back to the real render target through a custom
 * fragment shader that displaces the UVs radially. No overlay is painted,
 * so all content stays visible - it is only warped.
 */
class CursorWakeRippleEffect : public Effect
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kwin.CursorWakeRipple")

public:
    CursorWakeRippleEffect();
    ~CursorWakeRippleEffect() override;

    static bool supported();

    void reconfigure(ReconfigureFlags flags) override;
    void prePaintScreen(ScreenPrePaintData &data, std::chrono::milliseconds presentTime) override;
    void paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const QRegion &region, Output *screen) override;
    void postPaintScreen() override;
    bool isActive() const override;
    int requestedEffectChainPosition() const override
    {
        // Must run EARLY like zoom (10): we call effects->paintScreen() into an
        // offscreen FBO to capture the rest of the scene, then draw that texture
        // back. A late position clears a mostly-finished frame and looks like an
        // opaque cover.
        return 10;
    }

public Q_SLOTS:
    /// D-Bus: qdbus org.kde.KWin /CursorWakeRipple org.kde.kwin.CursorWakeRipple.trigger
    Q_SCRIPTABLE void trigger();
    /// D-Bus: ripple at an explicit global logical position.
    Q_SCRIPTABLE void triggerAt(double x, double y);

private Q_SLOTS:
    void slotMouseChanged(const QPointF &pos, const QPointF &oldpos,
                          Qt::MouseButtons buttons, Qt::MouseButtons oldbuttons,
                          Qt::KeyboardModifiers modifiers, Qt::KeyboardModifiers oldmodifiers);

private:
    struct OffscreenData
    {
        std::unique_ptr<GLTexture> texture;
        std::unique_ptr<GLFramebuffer> framebuffer;
        ColorDescription color = ColorDescription::sRGB;
    };

    void start(const QPointF &globalPos);
    bool ensureShader();
    OffscreenData *ensureOffscreen(const RenderTarget &renderTarget, Output *screen);

    // config (kwinrc [Effect-cursorwakeripple])
    int m_idleTimeoutMs = 1000; // Max: 1000 while testing, ~30000 later
    int m_durationMs = 1000;
    double m_amplitude = 0.012;
    double m_maxRadius = 0.18;
    double m_wavelength = 0.035;
    double m_ringWidth = 0.05;
    bool m_flipY = true;

    // state
    QElapsedTimer m_sinceLastMove;
    bool m_seenMove = false;
    bool m_running = false;
    QPointF m_center; // global logical coords
    Output *m_screen = nullptr;
    std::chrono::milliseconds m_startTime = std::chrono::milliseconds::zero();
    double m_progress = 0.0;

    std::unique_ptr<GLShader> m_shader;
    bool m_shaderFailed = false;
    std::map<Output *, OffscreenData> m_offscreen;
};

} // namespace KWin
