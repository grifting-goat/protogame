#version 330 core


uniform vec3 objectColor;
uniform sampler2D texSampler;
uniform bool useTexture;
uniform bool useLighting;

uniform vec3 lightDir;

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

void main()
{
    vec3 baseColor = useTexture
        ? texture(texSampler, TexCoord).rgb * objectColor
        : objectColor;

    if (!useLighting) {
        FragColor = vec4(baseColor, 1.0);
        return;
    }

    vec3 N = normalize(Normal);
    vec3 L = normalize(-lightDir); // negate if lightDir is "toward scene"

    float lightIntensity = max(dot(N, L), 0.0);
    float ambient = 0.50;
    float diffuse = 0.50 * lightIntensity;

    vec3 litColor = baseColor * (ambient + diffuse);
    FragColor = vec4(litColor, 1.0);
}