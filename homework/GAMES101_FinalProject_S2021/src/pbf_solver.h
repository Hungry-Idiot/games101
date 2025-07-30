#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "particle.h" // 确保 particle.h 也在 src 目录中
#include "shader.h"

enum class RenderMode {
    PARTICLES,
    FLUID
};

class PBFSolver {
public:
    ~PBFSolver();

    void initialize(RenderMode mode);
    void update(float dt);
    void render(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos);

private:
    void applyExternalForces(float dt);
    void findNeighbors();
    void solveConstraints();
    void updatePositionsAndVelocities(float dt);
    void enforceBoundary();

    // SPH 核函数
    float poly6_kernel(const glm::vec3& r, float h);
    glm::vec3 spiky_kernel_gradient(const glm::vec3& r, float h);
    
    void setupScreenQuad();
    void setupFBOs();
    void renderParticles(const glm::mat4& projection, const glm::mat4& view);
    void renderFluid(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos);

    // 成员变量
    std::vector<Particle> m_particles;
    RenderMode m_render_mode;

    // 模拟参数
    const float PARTICLE_RADIUS = 0.05f;
    const float KERNEL_RADIUS_H = PARTICLE_RADIUS * 4.0f; // h
    const float REST_DENSITY = 1000.0f;                  // ρ₀
    const int SOLVER_ITERATIONS = 4;
    const glm::vec3 GRAVITY = glm::vec3(0.0f, -9.8f, 0.0f);

    // 预计算的常数
    float m_poly6_const;
    float m_spiky_grad_const;

    // 粒子渲染
    unsigned int m_particles_vao = 0;
    unsigned int m_particles_vbo = 0;
    
    // 全屏四边形渲染
    unsigned int m_quad_vao = 0;
    unsigned int m_quad_vbo = 0;
    
    // 着色器程序
    Shader* m_particle_shader = nullptr; 
    Shader* m_depth_shader = nullptr;
    Shader* m_smooth_shader = nullptr;
    Shader* m_normals_shader = nullptr;
    Shader* m_final_shader = nullptr;

    // SSFR 帧缓冲和纹理
    unsigned int m_depth_fbo;
    unsigned int m_depth_texture;

    unsigned int m_smooth_fbo;
    unsigned int m_smooth_texture;

    unsigned int m_normals_fbo;
    unsigned int m_normals_texture;
};