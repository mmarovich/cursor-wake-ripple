// SPDX-License-Identifier: GPL-2.0-or-later
// GLES / legacy-GL variant of ripple_core.frag (KWin picks *_core.frag on core profiles).
#include "colormanagement.glsl"

uniform sampler2D sampler;

// Ripple parameters. All distances are in "texture-height units" (0..1 of
// the output height) after aspect correction so the ring stays circular.
uniform vec2 rippleCenter;   // ripple origin in texcoord space (0..1)
uniform float aspect;        // texture width / height
uniform float progress;      // 0 -> 1 over the animation
uniform float amplitude;     // max UV displacement (e.g. 0.012)
uniform float maxRadius;     // ring radius at progress == 1 (e.g. 0.18)
uniform float wavelength;    // distance between crests (e.g. 0.035)
uniform float ringWidth;     // gaussian width of the wave packet (e.g. 0.05)

vec2 rippleOffset(vec2 uv)
{
    vec2 d = uv - rippleCenter;
    d.x *= aspect;
    float dist = length(d);
    float front = progress * maxRadius;
    float x = dist - front;
    // Gaussian packet travelling outward, decaying as the animation ends.
    float envelope = exp(-(x * x) / (ringWidth * ringWidth));
    float fade = (1.0 - progress) * (1.0 - progress);
    float wave = sin(x / wavelength * 6.2831853);
    vec2 dir = dist > 0.00001 ? d / dist : vec2(0.0);
    vec2 offs = dir * wave * envelope * fade * amplitude;
    offs.x /= aspect;
    return offs;
}

varying vec2 texcoord0;

void main()
{
    vec2 uv = clamp(texcoord0 + rippleOffset(texcoord0), vec2(0.0), vec2(1.0));
    vec4 tex = texture2D(sampler, uv);
    tex = sourceEncodingToNitsInDestinationColorspace(tex);
    gl_FragColor = nitsToDestinationEncoding(tex);
}
