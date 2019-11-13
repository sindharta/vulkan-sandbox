#pragma once

#include <vulkan/vulkan.h> 
#include <glm/glm.hpp>
#include <vector>

struct TextureVertex {
    glm::vec2 Pos;
    glm::vec3 Color;
    glm::vec2 TexCoord;

    static const VkVertexInputBindingDescription* GetBindingDescription();
    static const std::vector<VkVertexInputAttributeDescription>* GetAttributeDescriptions();
};

//---------------------------------------------------------------------------------------------------------------------


