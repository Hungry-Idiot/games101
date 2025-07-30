#version 430 core
layout (location = 0) in vec3 aPos;

// 接收来自 C++ 的 MVP 矩阵
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // 将粒子位置通过 MVP 矩阵转换到最终的裁剪空间
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    gl_PointSize = 3.0; // 固定点的大小
}