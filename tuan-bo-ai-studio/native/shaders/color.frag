#version 440

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float exposure;
    float contrast;
    float highlights;
    float shadows;
    float whites;
    float blacks;
    float temperature;
    float tint;
    float saturation;
    float vibrance;
};

layout(binding = 1) uniform sampler2D source;

float satf(float x) { return clamp(x, 0.0, 1.0); }

void main() {
    vec4 px = texture(source, texCoord);
    vec3 c = px.rgb * exp2(exposure);

    float l = clamp(dot(c, vec3(0.2126, 0.7152, 0.0722)), 0.0, 1.0);
    float shW = clamp((0.60 - l) / 0.60, 0.0, 1.0);
    float hiW = clamp((l - 0.40) / 0.60, 0.0, 1.0);
    float blW = clamp((0.22 - l) / 0.22, 0.0, 1.0);
    float whW = clamp((l - 0.78) / 0.22, 0.0, 1.0);
    float tone = (shadows / 100.0) * 0.34 * shW
               + (highlights / 100.0) * 0.34 * hiW
               + (blacks / 100.0) * 0.22 * blW
               + (whites / 100.0) * 0.22 * whW;
    c += vec3(tone);

    float t = temperature / 100.0;
    float ti = tint / 100.0;
    c.r += t * 0.10 + ti * 0.025;
    c.g += ti * 0.055;
    c.b -= t * 0.10 + ti * 0.025;

    float con = 1.0 + contrast / 100.0;
    c = (c - 0.5) * con + 0.5;

    l = dot(c, vec3(0.2126, 0.7152, 0.0722));
    float maxc = max(c.r, max(c.g, c.b));
    float minc = min(c.r, min(c.g, c.b));
    float chroma = max(0.0, maxc - minc);
    float vibGain = 1.0 + (vibrance / 100.0) * (1.0 - min(1.0, chroma * 2.2));
    float satGain = max(0.0, 1.0 + saturation / 100.0) * vibGain;
    c = vec3(l) + (c - vec3(l)) * satGain;

    fragColor = vec4(clamp(c, 0.0, 1.0), px.a) * qt_Opacity;
}
