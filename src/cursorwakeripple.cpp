/*
    Cursor Wake Ripple - KWin 6 effect
    SPDX-FileCopyrightText: 2026 Max Marovich
    SPDX-License-Identifier: GPL-2.0-or-later

    Written against KWin 6.3 (Debian 13 "trixie" ships 4:6.3.6). KWin's effect
    API is not stable: in KWin master (6.5+/6.6) prePaintScreen() lost its
    presentTime arg and paintScreen()/RenderViewport signatures changed.
*/
#include "cursorwakeripple.h"

#include "core/output.h"
#include "core/rendertarget.h"
#include "core/renderviewport.h"
#include "effect/effecthandler.h"
#include "opengl/glframebuffer.h"
#include "opengl/glshader.h"
#include "opengl/glshadermanager.h"
#include "opengl/gltexture.h"
#include "opengl/glutils.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QDBusConnection>
#include <QLoggingCategory>
#include <QVector2D>

#include <algorithm>

static void ensureResources()
{
    // Pulls shaders out of the plugin's Qt resource blob (same pattern as invert/zoom).
    Q_INIT_RESOURCE(cursorwakeripple);
}

Q_LOGGING_CATEGORY(CURSORWAKERIPPLE, "kwin.effect.cursorwakeripple", QtWarningMsg)

namespace KWin
{

CursorWakeRippleEffect::CursorWakeRippleEffect()
{
    reconfigure(ReconfigureAll);

    connect(effects, &EffectsHandler::mouseChanged, this, &CursorWakeRippleEffect::slotMouseChanged);
    connect(effects, &EffectsHandler::screenRemoved, this, [this](Output *output) {
        m_offscreen.erase(output);
        if (m_screen == output) {
            m_running = false;
            m_screen = nullptr;
        }
    });

    QDBusConnection::sessionBus().registerObject(QStringLiteral("/CursorWakeRipple"),
                                                 this,
                                                 QDBusConnection::ExportScriptableSlots);
    m_sinceLastMove.start();
}

CursorWakeRippleEffect::~CursorWakeRippleEffect()
{
    QDBusConnection::sessionBus().unregisterObject(QStringLiteral("/CursorWakeRipple"));
}

bool CursorWakeRippleEffect::supported()
{
    return effects->isOpenGLCompositing() && effects->animationsSupported();
}

void CursorWakeRippleEffect::reconfigure(ReconfigureFlags)
{
    // Kept dependency-light on purpose (no KConfigXT/KCM yet). Tweak with e.g.
    //   kwriteconfig6 --file kwinrc --group Effect-cursorwakeripple --key IdleTimeoutMs 30000
    //   qdbus org.kde.KWin /Effects org.kde.kwin.Effects.reconfigureEffect cursorwakeripple
    const KConfigGroup cg = KSharedConfig::openConfig(QStringLiteral("kwinrc"))->group(QStringLiteral("Effect-cursorwakeripple"));
    m_idleTimeoutMs = std::max(0, cg.readEntry("IdleTimeoutMs", 1000));
    m_durationMs = std::clamp(cg.readEntry("DurationMs", 1000), 100, 5000);
    m_amplitude = cg.readEntry("Amplitude", 0.04);
    m_maxRadius = cg.readEntry("MaxRadius", 0.18);
    m_wavelength = std::max(0.001, cg.readEntry("Wavelength", 0.035));
    m_ringWidth = std::max(0.001, cg.readEntry("RingWidth", 0.05));
    m_flipY = cg.readEntry("FlipY", false);
}

void CursorWakeRippleEffect::slotMouseChanged(const QPointF &pos, const QPointF &oldpos,
                                              Qt::MouseButtons, Qt::MouseButtons,
                                              Qt::KeyboardModifiers, Qt::KeyboardModifiers)
{
    if (pos == oldpos) {
        return; // button/modifier-only change
    }
    const qint64 idle = m_sinceLastMove.restart();
    if (!m_seenMove) {
        m_seenMove = true; // arm after the first motion; avoid firing on session start
        return;
    }
    if (m_running) {
        // Keep the wave under the pointer while it plays.
        m_center = pos;
        if (Output *screen = effects->screenAt(pos.toPoint())) {
            m_screen = screen;
        }
        effects->addRepaintFull();
        return;
    }
    if (m_idleTimeoutMs > 0 && idle >= m_idleTimeoutMs) {
        start(pos);
    }
}

void CursorWakeRippleEffect::trigger()
{
    start(effects->cursorPos());
}

void CursorWakeRippleEffect::triggerAt(double x, double y)
{
    start(QPointF(x, y));
}

void CursorWakeRippleEffect::start(const QPointF &globalPos)
{
    if (effects->hasActiveFullScreenEffect() && effects->activeFullScreenEffect() != this) {
        return; // don't fight Overview / Desktop Grid etc.
    }
    Output *screen = effects->screenAt(globalPos.toPoint());
    if (!screen) {
        return;
    }
    m_center = globalPos;
    m_screen = screen;
    m_running = true;
    m_startTime = std::chrono::milliseconds::zero(); // latched on next prePaintScreen
    m_progress = 0.0;
    effects->addRepaintFull();
}

bool CursorWakeRippleEffect::isActive() const
{
    return m_running && !m_shaderFailed;
}

void CursorWakeRippleEffect::prePaintScreen(ScreenPrePaintData &data, std::chrono::milliseconds presentTime)
{
    if (m_running) {
        if (m_startTime == std::chrono::milliseconds::zero()) {
            m_startTime = presentTime;
        }
        const double elapsed = double((presentTime - m_startTime).count());
        m_progress = std::clamp(elapsed / double(m_durationMs), 0.0, 1.0);
        if (data.screen == m_screen) {
            // Forces a full-output repaint so the offscreen texture is complete.
            data.mask |= PAINT_SCREEN_TRANSFORMED;
        }
    }
    effects->prePaintScreen(data, presentTime);
}

bool CursorWakeRippleEffect::ensureShader()
{
    if (m_shader) {
        return true;
    }
    if (m_shaderFailed) {
        return false;
    }
    ensureResources();
    // KWin resolves ripple.frag -> ripple_core.frag on core-profile contexts and
    // expands #include "colormanagement.glsl" from libkwin's own resources.
    m_shader = ShaderManager::instance()->generateShaderFromFile(
        ShaderTrait::MapTexture,
        QString(),
        QStringLiteral(":/effects/cursorwakeripple/shaders/ripple.frag"));
    if (!m_shader || !m_shader->isValid()) {
        qCWarning(CURSORWAKERIPPLE) << "Failed to compile ripple shader; disabling effect";
        m_shader.reset();
        m_shaderFailed = true;
        return false;
    }
    // Debug only: this is a one-shot startup note, not a failure. Release builds stay quiet.
    qCDebug(CURSORWAKERIPPLE) << "Ripple shader ready";
    return true;
}

CursorWakeRippleEffect::OffscreenData *CursorWakeRippleEffect::ensureOffscreen(const RenderTarget &renderTarget, Output *screen)
{
    OffscreenData &data = m_offscreen[screen];
    const QSize nativeSize = renderTarget.size();
    data.color = renderTarget.colorDescription();
    const GLenum format = data.color == ColorDescription::sRGB ? GL_RGBA8 : GL_RGBA16F;
    if (!data.texture || data.texture->size() != nativeSize || data.texture->internalFormat() != format) {
        data.texture = GLTexture::allocate(format, nativeSize);
        if (!data.texture) {
            return nullptr;
        }
        data.texture->setFilter(GL_LINEAR);
        data.texture->setWrapMode(GL_CLAMP_TO_EDGE);
        data.framebuffer = std::make_unique<GLFramebuffer>(data.texture.get());
    }
    data.texture->setContentTransform(renderTarget.transform());
    return &data;
}

void CursorWakeRippleEffect::paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const QRegion &region, Output *screen)
{
    // Fast path: not animating, or a different monitor -> zero overhead.
    if (!m_running || screen != m_screen || !ensureShader()) {
        effects->paintScreen(renderTarget, viewport, mask, region, screen);
        return;
    }
    OffscreenData *off = ensureOffscreen(renderTarget, screen);
    if (!off) {
        effects->paintScreen(renderTarget, viewport, mask, region, screen);
        return;
    }

    // 1) Composite the whole output (all windows, wallpaper, panels) offscreen.
    RenderTarget offTarget(off->framebuffer.get(), renderTarget.colorDescription());
    RenderViewport offViewport(viewport.renderRect(), viewport.scale(), offTarget);
    GLFramebuffer::pushFramebuffer(off->framebuffer.get());
    effects->paintScreen(offTarget, offViewport, mask, region, screen);
    GLFramebuffer::popFramebuffer();

    // 2) Draw it back through the ripple shader (full-screen quad, opaque copy
    //    of the real content - nothing is covered, pixels are only displaced).
    const QRectF geo = viewport.renderRect();
    const qreal scale = viewport.scale();
    QPointF n((m_center.x() - geo.x()) / geo.width(), (m_center.y() - geo.y()) / geo.height());
    if (m_flipY) {
        n.setY(1.0 - n.y()); // GL framebuffer textures are bottom-up
    }
    if (m_progress < 0.05) {
        // Debug only: this runs on early animation frames and used to spam qCWarning (a lag source).
        qCDebug(CURSORWAKERIPPLE) << "center" << m_center << "geo" << geo << "uv" << n << "flipY" << m_flipY
                                  << "texSize" << off->texture->size() << "rtSize" << renderTarget.size();
    }

    // Same MVP construction as zoom: projection * translate(viewport origin in device px).
    QMatrix4x4 matrix;
    matrix.translate(geo.x() * scale, geo.y() * scale);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    ShaderManager::instance()->pushShader(m_shader.get());
    m_shader->setUniform(GLShader::Mat4Uniform::ModelViewProjectionMatrix, viewport.projectionMatrix() * matrix);
    m_shader->setUniform(GLShader::IntUniform::TextureWidth, off->texture->width());
    m_shader->setUniform(GLShader::IntUniform::TextureHeight, off->texture->height());
    m_shader->setColorspaceUniforms(off->color, renderTarget.colorDescription(), RenderingIntent::Perceptual);
    m_shader->setUniform("rippleCenter", QVector2D(n));
    m_shader->setUniform("aspect", float(geo.width() / geo.height()));
    m_shader->setUniform("progress", float(m_progress));
    m_shader->setUniform("amplitude", float(m_amplitude));
    m_shader->setUniform("maxRadius", float(m_maxRadius));
    m_shader->setUniform("wavelength", float(m_wavelength));
    m_shader->setUniform("ringWidth", float(m_ringWidth));

    off->texture->render(geo.size() * scale);
    ShaderManager::instance()->popShader();
}

void CursorWakeRippleEffect::postPaintScreen()
{
    if (m_running) {
        if (m_progress >= 1.0) {
            m_running = false;
            m_screen = nullptr;
            m_offscreen.clear(); // free VRAM between ripples
        }
        effects->addRepaintFull();
    }
    effects->postPaintScreen();
}

} // namespace KWin

#include "moc_cursorwakeripple.cpp"
