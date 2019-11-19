#include "CudaImage.h"

#include <stdexcept> //std::runtime_error
#include <sstream> //ostringstream

#include "Shin/Texture.h"
#include "Shin/Utilities/GraphicsUtility.h"

//---------------------------------------------------------------------------------------------------------------------

CudaImage::CudaImage() : m_array(nullptr), m_mipmapArray(nullptr), m_extMemory(nullptr)
{
}

//---------------------------------------------------------------------------------------------------------------------

CudaImage::~CudaImage()
{
}

//---------------------------------------------------------------------------------------------------------------------

void CudaImage::Init(const VkDevice device, const Shin::Texture* texture)
{
    int fd = -1;
    CUresult result = CUDA_SUCCESS;

    void *p = GraphicsUtility::GetExportHandle(device, texture->GetTextureImageMemory());

    if (nullptr == p) {
        throw std::runtime_error("Failed to get export handle for memory");
    }

    CUDA_EXTERNAL_MEMORY_HANDLE_DESC memDesc = {};
#ifndef _WIN32
    memDesc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD;
#else
    memDesc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32;
#endif
    memDesc.handle.fd = (int)(uintptr_t)p;
    memDesc.size = texture->GetTextureImageMemorySize();

    if ((result=cuImportExternalMemory(&m_extMemory, &memDesc)) != CUDA_SUCCESS) {
        throw std::runtime_error("Failed to import buffer into CUDA");
    }

    VkExtent2D extent = texture->GetExtent();

    CUDA_ARRAY3D_DESCRIPTOR arrayDesc = {};
    arrayDesc.Width = extent.width;
    arrayDesc.Height = extent.height;
    arrayDesc.Depth = 0; /* CUDA 2D arrays are defined to have depth 0 */
    arrayDesc.Format = CU_AD_FORMAT_UNSIGNED_INT8;
    arrayDesc.NumChannels = 1;
    arrayDesc.Flags = CUDA_ARRAY3D_SURFACE_LDST |
                      CUDA_ARRAY3D_COLOR_ATTACHMENT;

    CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC mipmapArrayDesc = {};
    mipmapArrayDesc.arrayDesc = arrayDesc;
    mipmapArrayDesc.numLevels = 1;

    result = cuExternalMemoryGetMappedMipmappedArray(&m_mipmapArray, m_extMemory,
                 &mipmapArrayDesc);
    if (result != CUDA_SUCCESS) {
        std::ostringstream oss;
        oss << "Failed to get CUmipmappedArray; " << result;
        throw std::runtime_error(oss.str());
    }

    result = cuMipmappedArrayGetLevel(&m_array, m_mipmapArray, 0);
    if (result != CUDA_SUCCESS) {
        std::ostringstream oss;
        oss << "Failed to get CUarray; " << result;
        throw std::runtime_error(oss.str());
    }
}

//---------------------------------------------------------------------------------------------------------------------

void CudaImage::CleanUp() {
    m_array = nullptr;

    if (nullptr != m_mipmapArray) {
        cuMipmappedArrayDestroy(m_mipmapArray);
        m_mipmapArray = nullptr;
    }
    if (nullptr != m_extMemory) {
        cuDestroyExternalMemory(m_extMemory);
        m_extMemory = nullptr;
    }


}
