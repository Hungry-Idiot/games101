#include "pbf_solver.h"
#include <glad/glad.h>
#include <cmath>
#include <iostream>
#include <glm/gtc/type_ptr.hpp> // for glm::value_ptr
#include "config.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PBFSolver::~PBFSolver() {
    delete m_particle_shader;
    delete m_depth_shader;
    delete m_smooth_shader;
    delete m_normals_shader;
    delete m_final_shader;

    glDeleteVertexArrays(1, &m_particles_vao);
    glDeleteBuffers(1, &m_particles_vbo);
    glDeleteVertexArrays(1, &m_quad_vao);
    glDeleteBuffers(1, &m_quad_vbo);

    if (m_render_mode == RenderMode::FLUID) {
        glDeleteFramebuffers(1, &m_depth_fbo);
        glDeleteTextures(1, &m_depth_texture);
        glDeleteFramebuffers(1, &m_smooth_fbo);
        glDeleteTextures(1, &m_smooth_texture);
        glDeleteFramebuffers(1, &m_normals_fbo);
        glDeleteTextures(1, &m_normals_texture);
    }
}

void PBFSolver::initialize(RenderMode mode) {
    m_render_mode = mode;

    m_poly6_const = 315.0f / (64.0f * M_PI * pow(KERNEL_RADIUS_H, 9));
    m_spiky_grad_const = -45.0f / (M_PI * pow(KERNEL_RADIUS_H, 6));

    std::cout << "Initializing particles..." << std::endl;
    // 扩大x,y,z的范围来生成更多粒子
    for (int x = -10; x < 10; ++x) {
        for (int y = 0; y < 30; ++y) { // 增加高度
            for (int z = -5; z < 5; ++z) {
                Particle p;
                // 调整初始位置和分布
                p.position = glm::vec3(x, y, z) * PARTICLE_RADIUS * 1.2f + glm::vec3(0.0f, 1.0f, 0.0f);
                p.velocity = glm::vec3(0.0f);
                m_particles.push_back(p);
            }
        }
    }
    std::cout << "Initialized " << m_particles.size() << " particles." << std::endl;

    std::string vert_path = std::string(PROJECT_ROOT_DIR) + "/shaders/particle.vert";
    std::string frag_path = std::string(PROJECT_ROOT_DIR) + "/shaders/particle.frag";
    std::cout << "Loading vertex shader from: " << vert_path << std::endl;
    std::cout << "Loading fragment shader from: " << frag_path << std::endl;

    // --- 加载着色器的代码现在完全不变，但更可靠了 ---
    try {
        if (m_render_mode == RenderMode::PARTICLES) {
            std::string vert_path = std::string(PROJECT_ROOT_DIR) + "/shaders/particle.vert";
            std::string frag_path = std::string(PROJECT_ROOT_DIR) + "/shaders/particle.frag";
            m_particle_shader = new Shader(vert_path.c_str(), frag_path.c_str());
        } else { // FLUID 模式
            m_depth_shader = new Shader("shaders/particle_depth.vert", "shaders/particle_depth.frag");
            m_smooth_shader = new Shader("shaders/fullscreen.vert", "shaders/smooth.frag");
            m_normals_shader = new Shader("shaders/fullscreen.vert", "shaders/normals.frag");
            m_final_shader = new Shader("shaders/fullscreen.vert", "shaders/final.frag");
        }
    }
    catch (const std::ifstream::failure& e) {
        std::cerr << "FATAL ERROR: Shader file not found or could not be read. " << e.what() << std::endl;
        throw std::runtime_error("Shader file loading failed.");
    }

    if (!m_particle_shader || m_particle_shader->ID == 0) {
        std::cerr << "FATAL ERROR: Shader program creation or linking failed." << std::endl;
        throw std::runtime_error("Shader creation failed.");
    }
    
    // --- VAO/VBO 设置 (保持不变) ...
    glGenVertexArrays(1, &m_particles_vao);
    glGenBuffers(1, &m_particles_vbo);
    glBindVertexArray(m_particles_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_particles_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_particles.size() * sizeof(Particle), m_particles.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, position));
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    if (m_render_mode == RenderMode::FLUID) {
        setupScreenQuad();
        setupFBOs();
    }
}

void PBFSolver::update(float dt) {
    if (m_particles.empty()) return;

    applyExternalForces(dt);
    findNeighbors(); // 暂时是 O(N^2)
    solveConstraints();
    updatePositionsAndVelocities(dt);
}

// 简单的CPU渲染，直接绘制点
void PBFSolver::render(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos) {
    if (m_render_mode == RenderMode::PARTICLES) {
        renderParticles(projection, view);
    } else {
        renderFluid(projection, view, cameraPos);
    }
}

void PBFSolver::setupScreenQuad() {
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    glGenVertexArrays(1, &m_quad_vao);
    glGenBuffers(1, &m_quad_vbo);
    glBindVertexArray(m_quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void PBFSolver::setupFBOs() {
    // 假设窗口大小为 1280x720，实际项目中应从外部获取
    const unsigned int SCR_WIDTH = 1280;
    const unsigned int SCR_HEIGHT = 720;

    auto createFBOWithTexture = [&](unsigned int& fbo, unsigned int& texture, GLint format) {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, format, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    };
    
    // 为深度、平滑、法线创建 FBO
    createFBOWithTexture(m_depth_fbo, m_depth_texture, GL_R32F);
    createFBOWithTexture(m_smooth_fbo, m_smooth_texture, GL_R32F);
    createFBOWithTexture(m_normals_fbo, m_normals_texture, GL_RGB16F); // 法线需要RGB通道

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// 粒子渲染（之前的render函数）
void PBFSolver::renderParticles(const glm::mat4& projection, const glm::mat4& view) {
    if (m_particles.empty() || !m_particle_shader) return;

    m_particle_shader->use();
    m_particle_shader->setMat4("projection", projection);
    m_particle_shader->setMat4("view", view);
    m_particle_shader->setMat4("model", glm::mat4(1.0f));
    
    glBindBuffer(GL_ARRAY_BUFFER, m_particles_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_particles.size() * sizeof(Particle), m_particles.data());
    
    glBindVertexArray(m_particles_vao);
    glDrawArrays(GL_POINTS, 0, m_particles.size());
    glBindVertexArray(0);
}

// 流体渲染（SSFR多通道）
void PBFSolver::renderFluid(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos) {
    if (m_particles.empty()) return;

    // --- Pass 1: 渲染粒子深度 ---
    glBindFramebuffer(GL_FRAMEBUFFER, m_depth_fbo);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 每次绑定FBO都清理
    m_depth_shader->use();
    m_depth_shader->setMat4("projection", projection);
    m_depth_shader->setMat4("view", view);
    m_depth_shader->setFloat("particleRadius", PARTICLE_RADIUS);
    glBindVertexArray(m_particles_vao);
    glDrawArrays(GL_POINTS, 0, m_particles.size());

    // --- Pass 2: 平滑深度图 ---
    glBindFramebuffer(GL_FRAMEBUFFER, m_smooth_fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_smooth_shader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_depth_texture);
    m_smooth_shader->setInt("depthTexture", 0);
    glBindVertexArray(m_quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // --- Pass 3: 重建法线 ---
    glBindFramebuffer(GL_FRAMEBUFFER, m_normals_fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_normals_shader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_smooth_texture);
    m_normals_shader->setInt("smoothedDepthTexture", 0);
    m_normals_shader->setMat4("projection", projection);
    glBindVertexArray(m_quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // --- Pass 4: 最终着色 ---
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // 渲染回主窗口
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_final_shader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_smooth_texture);
    m_final_shader->setInt("smoothedDepthTexture", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_normals_texture);
    m_final_shader->setInt("normalTexture", 1);
    // skybox 等其他纹理也可以在这里绑定
    m_final_shader->setMat4("projection", projection);
    m_final_shader->setMat4("view", view);
    m_final_shader->setVec3("cameraPos", cameraPos);
    glBindVertexArray(m_quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
}

// --- 私有辅助函数实现 ---

void PBFSolver::applyExternalForces(float dt) {
    for (auto& p : m_particles) {
        p.velocity += GRAVITY * dt;
        p.predicted_position = p.position + p.velocity * dt;
    }
}

void PBFSolver::findNeighbors() {
    for (size_t i = 0; i < m_particles.size(); ++i) {
        m_particles[i].neighbors.clear();
        for (size_t j = 0; j < m_particles.size(); ++j) {
            if (i == j) continue;
            glm::vec3 diff = m_particles[i].predicted_position - m_particles[j].predicted_position;
            if (glm::length(diff) < KERNEL_RADIUS_H) {
                m_particles[i].neighbors.push_back(j);
            }
        }
    }
}

void PBFSolver::solveConstraints() {
     for (int iter = 0; iter < SOLVER_ITERATIONS; ++iter) {
        // a. 计算每个粒子的 lambda
        for (size_t i = 0; i < m_particles.size(); ++i) {
            Particle& p_i = m_particles[i];
            
            float density = 0.0f;
            for (int neighbor_idx : p_i.neighbors) {
                const Particle& p_j = m_particles[neighbor_idx];
                density += poly6_kernel(p_i.predicted_position - p_j.predicted_position, KERNEL_RADIUS_H);
            }

            float C_i = (density / REST_DENSITY) - 1.0f;
            
            float sum_grad_C_sq = 0.0f;
            glm::vec3 grad_C_i_sum = glm::vec3(0.0f);

            for (int neighbor_idx : p_i.neighbors) {
                const Particle& p_j = m_particles[neighbor_idx];
                glm::vec3 grad_C_j = (1.0f / REST_DENSITY) * spiky_kernel_gradient(p_i.predicted_position - p_j.predicted_position, KERNEL_RADIUS_H);
                sum_grad_C_sq += glm::dot(grad_C_j, grad_C_j);
                grad_C_i_sum -= grad_C_j; // 注意这里是减号
            }
            sum_grad_C_sq += glm::dot(grad_C_i_sum, grad_C_i_sum);

            const float EPSILON = 1e-6f;
            p_i.lambda = -C_i / (sum_grad_C_sq + EPSILON);
        }

        // b. 计算位置修正 Δp
        for (size_t i = 0; i < m_particles.size(); ++i) {
            Particle& p_i = m_particles[i];
            p_i.delta_p = glm::vec3(0.0f);
            for (int neighbor_idx : p_i.neighbors) {
                const Particle& p_j = m_particles[neighbor_idx];
                
                // s_corr (暂时禁用以简化)
                // float s_corr = 0.0f; 
                p_i.delta_p += (p_i.lambda + p_j.lambda) * spiky_kernel_gradient(p_i.predicted_position - p_j.predicted_position, KERNEL_RADIUS_H);
            }
            p_i.delta_p /= REST_DENSITY;
        }

        // c. 应用位置修正
        for(auto& p : m_particles) {
            p.predicted_position += p.delta_p;
        }

        enforceBoundary();
    }
}

void PBFSolver::updatePositionsAndVelocities(float dt) {
    for (auto& p : m_particles) {
        p.velocity = (p.predicted_position - p.position) / dt;
        p.position = p.predicted_position;
    }
}

void PBFSolver::enforceBoundary() {
    // 简单的立方体容器
    const float bound = 2.0f;
    for(auto& p : m_particles) {
        p.predicted_position.x = glm::clamp(p.predicted_position.x, -bound, bound);
        p.predicted_position.y = glm::max(0.0f, p.predicted_position.y);
        p.predicted_position.z = glm::clamp(p.predicted_position.z, -bound, bound);
    }
}

// SPH 核函数实现
float PBFSolver::poly6_kernel(const glm::vec3& r, float h) {
    float r_len_sq = glm::dot(r, r);
    float h_sq = h * h;
    if (r_len_sq > h_sq) return 0.0f;
    return m_poly6_const * pow(h_sq - r_len_sq, 3);
}

glm::vec3 PBFSolver::spiky_kernel_gradient(const glm::vec3& r, float h) {
    float r_len = glm::length(r);
    if (r_len > h || r_len == 0.0f) return glm::vec3(0.0f);
    float h_minus_r = h - r_len;
    return m_spiky_grad_const * h_minus_r * h_minus_r * (r / r_len);
}