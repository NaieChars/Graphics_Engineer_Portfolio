#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D colorBuffer;

void main()
{
    vec4 color = texture(colorBuffer, TexCoords);
    float brightness = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    if(brightness > 0.1)  // HDR 场景阈值调高
        FragColor = color;
    else
        FragColor = vec4(0.0);
}