#include "ColorVertex.h"

//---------------------------------------------------------------------------------------------------------------------

const VkVertexInputBindingDescription* ColorVertex::GetBindingDescription() {

    static VkVertexInputBindingDescription bindingDescription = {
            0, sizeof(ColorVertex), VK_VERTEX_INPUT_RATE_VERTEX
    }; 
    return &bindingDescription;
}


//---------------------------------------------------------------------------------------------------------------------

const std::vector<VkVertexInputAttributeDescription>* ColorVertex::GetAttributeDescriptions() {

    static std::vector<VkVertexInputAttributeDescription> attributeDescriptions{ 
        { 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ColorVertex, Pos) },
        { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ColorVertex, Color) },
    }; 
    return &attributeDescriptions;
}
