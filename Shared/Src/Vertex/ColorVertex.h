#pragma once

#include <glm/glm.hpp>
#include <array>
struct ColorVertex {
    glm::vec2 Pos;
    glm::vec3 Color;

//---------------------------------------------------------------------------------------------------------------------

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(ColorVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; //not instanced rendering
        return bindingDescription;
    }

//---------------------------------------------------------------------------------------------------------------------

    static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(ColorVertex, Pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(ColorVertex, Color);

        return attributeDescriptions;
    }

};

