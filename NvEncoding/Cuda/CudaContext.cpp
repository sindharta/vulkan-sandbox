#include "CudaContext.h"

#include <stdexcept> //std::runtime_error
#include <array>
#include "Shin/Utilities/GraphicsUtility.h"

CudaContext::CudaContext() : m_context(nullptr) {
}

//---------------------------------------------------------------------------------------------------------------------

CudaContext::~CudaContext() {
}

//---------------------------------------------------------------------------------------------------------------------

void CudaContext::Init(const VkInstance instance, VkPhysicalDevice physicalDevice) {

    CUdevice dev;
    CUresult result = CUDA_SUCCESS;
    bool foundDevice = true;

    result = cuInit(0);
    if (result != CUDA_SUCCESS) {
        throw std::runtime_error("Failed to cuInit()");
    }

    int numDevices = 0;
    result = cuDeviceGetCount(&numDevices);
    if (result != CUDA_SUCCESS) {
        throw std::runtime_error("Failed to get count of CUDA devices");
    }

    CUuuid id = {};
    std::array<uint8_t, VK_UUID_SIZE> deviceUUID;
    GraphicsUtility::GetPhysicalDeviceUUIDInto(instance, physicalDevice, &deviceUUID);

    /*
     * Loop over the available devices and identify the CUdevice
     * corresponding to the physical device in use by this Vulkan instance.
     * This is required because there is no other way to match GPUs across
     * API boundaries.
     */
    for (int i = 0; i < numDevices; i++) {
        cuDeviceGet(&dev, i);

        cuDeviceGetUuid(&id, dev);

        if (!std::memcmp(static_cast<const void *>(&id),
                static_cast<const void *>(deviceUUID.data()),
                sizeof(CUuuid))) {
            foundDevice = true;
            break;
        }
    }

    if (!foundDevice) {
        throw std::runtime_error("Failed to get an appropriate CUDA device");
 
 }

    result = cuCtxCreate(&m_context, 0, dev);
    if (result != CUDA_SUCCESS) {
        throw std::runtime_error("Failed to create a CUDA context");
    }
}

//---------------------------------------------------------------------------------------------------------------------

void CudaContext::CleanUp() {
    if (nullptr != m_context) {
        cuCtxDestroy(m_context);
        m_context = nullptr;
    }
}
