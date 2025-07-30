#version 430 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D smoothedDepthTexture;
uniform sampler2D normalTexture;
uniform samplerCube skybox; // 天空盒立方体贴图
uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPos;

// 从深度重建世界坐标 (与normals.frag中一致)
vec3 worldPosFromDepth(float depth, vec2 texCoords) {
    float z = depth * 2.0 - 1.0;
    vec4 clipSpacePos = vec4(texCoords * 2.0 - 1.0, z, 1.0);
    vec4 worldPos = inverse(projection) * clipSpacePos;
    return worldPos.xyz / worldPos.w;
}

void main() {
    float depth = texture(smoothedDepthTexture, TexCoords).r;
    if (depth >= 1.0) {
        discard; // 或者渲染天空盒
    }

    vec3 worldPos = worldPosFromDepth(depth, TexCoords);
    vec3 normal = normalize(texture(normalTexture, TexCoords).rgb);

    // --- 光照 ---
    vec3 lightPos = vec3(10.0, 20.0, 10.0);
    vec3 lightColor = vec3(1.0);
    vec3 ambient = 0.1 * lightColor;

    vec3 lightDir = normalize(lightPos - worldPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // --- 反射 ---
    vec3 viewDir = normalize(cameraPos - worldPos);
    vec3 reflectDir = reflect(-viewDir, normal);
    vec3 reflectionColor = texture(skybox, reflectDir).rgb;

    // --- 菲涅尔效应 ---
    float fresnel = 0.04 + (1.0 - 0.04) * pow(1.0 - max(dot(viewDir, normal), 0.0), 5.0);

    // --- 颜色 ---
    vec3 waterColor = vec3(0.1, 0.3, 0.5); // 水的基色
    vec3 color = mix(waterColor + ambient + diffuse, reflectionColor, fresnel);

    FragColor = vec4(color, 1.0);
}