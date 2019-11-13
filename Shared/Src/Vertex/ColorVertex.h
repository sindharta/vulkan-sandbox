#pragma once

#include <vulkan/vulkan.h> 
#include <vector>
#include <glm/glm.hpp>

struct ColorVertex {
    glm::vec2 Pos;
    glm::vec3 Color;

    static const VkVertexInputBindingDescription* GetBindingDescription();
    static const std::vector<VkVertexInputAttributeDescription>* GetAttributeDescriptions();

};

