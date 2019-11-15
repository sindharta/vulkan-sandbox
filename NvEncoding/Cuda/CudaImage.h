#pragma once

#include "cuda.h"
#include <vulkan/vulkan.h>

namespace Shin {
    class Texture;
}

/**
*  @brief Wrapper class around CUarray
* This class can be used for mapping a 2D CUDA array on the device memory
* object referred to by deviceMem. deviceMem should have been created with a
* device memory object backing a 2D VkImage. This mapping makes use of Vulkan's
* export of device memory followed by import of this external memory by CUDA.
*/
class CudaImage
{
public:
    CudaImage();
    ~CudaImage();
    void Init(const VkDevice device, const Shin::Texture* texture);
    void CleanUp();


private:
    CUarray m_array;
    CUmipmappedArray m_mipmapArray;
    CUexternalMemory m_extMemory;


};
