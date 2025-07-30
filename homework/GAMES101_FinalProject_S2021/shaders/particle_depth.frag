#version 430 core
layout (location = 0) out float fragDepth;

// 新增输入变量
in float viewDepth;

uniform float particleRadius;

void main() {
    // 计算点精灵内部的距离
    vec2 pointCoord = gl_PointCoord - vec2(0.5);
    float distSq = dot(pointCoord, pointCoord);
    
    // 如果在圆形外，则抛弃
    if (distSq > 0.25) {
        discard;
    }

    // --- 精确的球体深度计算 ---
    // 1. 计算片元在球体表面上的偏移量
    // 这里的 z_offset 是在“点精灵空间”的偏移
    float z_offset = sqrt(0.25 - distSq);

    // 2. 将偏移量转换为视图空间的深度偏移
    // gl_PointSize 是点在屏幕上的像素大小
    // gl_Position.w 是裁剪空间的w分量，用于透视校正
    float viewSpaceDepthOffset = z_offset * gl_PointSize / (gl_Position.w / particleRadius);

    // 3. 计算最终的、线性的视图空间深度
    // 我们将偏移量加到粒子中心的深度上 (因为viewDepth是负值)
    float finalViewDepth = viewDepth + viewSpaceDepthOffset;

    // ‼️ 将线性的视图深度写入输出
    // 我们通常存储正值，所以取个反
    fragDepth = -finalViewDepth;
}