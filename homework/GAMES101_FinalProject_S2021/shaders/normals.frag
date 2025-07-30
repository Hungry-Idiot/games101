#version 430 core
out vec3 fragNormal;
in vec2 TexCoords;

uniform sampler2D smoothedDepthTexture;
uniform mat4 projection;
uniform vec2 screenSize;

// 从深度重建世界坐标
vec3 worldPosFromDepth(float depth, vec2 texCoords) {
    float z = depth * 2.0 - 1.0; // NDC z
    vec4 clipSpacePos = vec4(texCoords * 2.0 - 1.0, z, 1.0);
    vec4 worldPos = inverse(projection) * clipSpacePos;
    return worldPos.xyz / worldPos.w;
}

void main() {
    float centerDepth = texture(smoothedDepthTexture, TexCoords).r;
    if (centerDepth >= 1.0) {
        discard;
    }

    vec2 texelSize = 1.0 / screenSize;

    // 采样邻近像素深度
    float depthUp = texture(smoothedDepthTexture, TexCoords + vec2(0.0, texelSize.y)).r;
    float depthDown = texture(smoothedDepthTexture, TexCoords - vec2(0.0, texelSize.y)).r;
    float depthRight = texture(smoothedDepthTexture, TexCoords + vec2(texelSize.x, 0.0)).r;
    float depthLeft = texture(smoothedDepthTexture, TexCoords - vec2(texelSize.x, 0.0)).r;

    // 重建世界坐标
    vec3 centerPos = worldPosFromDepth(centerDepth, TexCoords);
    vec3 upPos = worldPosFromDepth(depthUp, TexCoords + vec2(0.0, texelSize.y));
    vec3 rightPos = worldPosFromDepth(depthRight, TexCoords + vec2(texelSize.x, 0.0));
    
    // 计算法线
    vec3 normal = normalize(cross(rightPos - centerPos, upPos - centerPos));
    
    fragNormal = normal;
}