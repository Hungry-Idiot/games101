#version 430 core
out float fragColor;
in vec2 TexCoords;

uniform sampler2D depthTexture;
uniform vec2 screenSize;

// 双边滤波参数
const float blurRadius = 3.0; // 模糊半径（像素）
const float sigmaDepth = 0.01; // 深度相似性因子
const float sigmaSpace = 5.0;  // 空间距离因子

void main() {
    vec2 texelSize = 1.0 / screenSize;
    float centerDepth = texture(depthTexture, TexCoords).r;

    // 如果背景深度为1.0，则不处理
    if (centerDepth >= 1.0) {
        fragColor = centerDepth;
        return;
    }
    
    float totalWeight = 0.0;
    float blurredDepth = 0.0;
    
    for (float x = -blurRadius; x <= blurRadius; x++) {
        for (float y = -blurRadius; y <= blurRadius; y++) {
            vec2 offset = vec2(x, y) * texelSize;
            float sampleDepth = texture(depthTexture, TexCoords + offset).r;
            
            if (sampleDepth < 1.0) {
                // 计算权重
                float depthDiff = sampleDepth - centerDepth;
                float weightDepth = exp(-(depthDiff * depthDiff) / (2.0 * sigmaDepth * sigmaDepth));
                
                float spaceDistSq = x*x + y*y;
                float weightSpace = exp(-spaceDistSq / (2.0 * sigmaSpace * sigmaSpace));
                
                float weight = weightDepth * weightSpace;
                
                totalWeight += weight;
                blurredDepth += sampleDepth * weight;
            }
        }
    }
    
    if (totalWeight > 0.0) {
        fragColor = blurredDepth / totalWeight;
    } else {
        fragColor = centerDepth;
    }
}