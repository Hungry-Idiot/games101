#version 430 core
layout (location = 0) in vec3 aPos; // 粒子中心位置

uniform mat4 projection;
uniform mat4 view;
uniform float particleRadius;

// 新增输出变量
out float viewDepth;

void main() {
    // 将粒子位置转换到视图空间
    vec4 viewPos = view * vec4(aPos, 1.0);

    viewDepth = viewPos.z;

    // 裁剪空间位置计算不变
    gl_Position = projection * viewPos;

    // 点大小计算不变
    gl_PointSize = particleRadius * 500.0 / gl_Position.w;
}