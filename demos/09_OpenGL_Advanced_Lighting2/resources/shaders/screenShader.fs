#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float exposure;  // 曝光度


void main()
{
   vec3 hdrColor = texture(screenTexture, TexCoords).rgb;

    // Reinhard 色调映射
    // vec3 mapped = hdrColor / (hdrColor + vec3(1.0));

    // 曝光色调映射（效果更好）
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);

    // Gamma 矫正（已经开了 GL_FRAMEBUFFER_SRGB，这行可以不加）
    // mapped = pow(mapped, vec3(1.0 / 2.2));

    FragColor = vec4(mapped, 1.0);
} 

