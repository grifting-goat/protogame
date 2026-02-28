#version 330 core


uniform vec3 objectColor;
uniform sampler2D texSampler;
uniform bool useTexture;

in vec2 TexCoord; 
out vec4 FragColor;

void main()
{
    if (useTexture) {
        vec4 texColor = texture(texSampler, TexCoord);
        FragColor = texColor * vec4(objectColor, 1.0);
    } else {
        FragColor = vec4(objectColor, 1.0);
    }
}