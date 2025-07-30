#pragma once
#include <glm/glm.hpp>
#include <vector>

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 predicted_position;
    glm::vec3 delta_p = glm::vec3(0.0f);
    float lambda = 0.0f;
    
    // 存储邻居的索引，这在CPU版本中很有用
    std::vector<int> neighbors; 
};