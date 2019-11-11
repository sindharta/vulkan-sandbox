#include "TextureVertex.h"


//---------------------------------------------------------------------------------------------------------------------

const VkVertexInputBindingDescription* TextureVertex::GetBindingDescription() {
    static VkVertexInputBindingDescription bindingDescription = {
            0, sizeof(TextureVertex), VK_VERTEX_INPUT_RATE_VERTEX
    }; 
    return &bindingDescription;
}


//---------------------------------------------------------------------------------------------------------------------

const std::vector<VkVertexInputAttributeDescription>* TextureVertex::GetAttributeDescriptions() {
    static std::vector<VkVertexInputAttributeDescription> attributeDescriptions{ 
        { 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(TextureVertex, Pos) },
        { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TextureVertex, Color) },
        { 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(TextureVertex, TexCoord) },

    }; 
    return &attributeDescriptions;
}
