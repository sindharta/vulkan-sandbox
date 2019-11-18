#pragma once

#include <vulkan/vulkan.h>
#include <cuda.h>

/**
*  @brief Wrapper class around CUcontext
* This class can be used for creating CUDA contexts on the device referenced
* by the provided Vkdev instance. The methods provided in this class are
* wrappers over CUDA API calls and take care of pushing/popping the CUDA
* context as required.
*/
class CudaContext
{
public:
    CudaContext();
    ~CudaContext();

    void Init(const VkInstance instance, VkPhysicalDevice physicalDevice);
    void CleanUp();
    inline const CUcontext GetContext() const;
private:
    CUcontext m_context;

};

inline const CUcontext CudaContext::GetContext() const { return m_context; }
