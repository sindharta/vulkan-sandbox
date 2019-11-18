#pragma once
#include "nvEncodeAPI.h"
#include <vector>
#include "cuda.h"


class NvEncoder {
public:
    NvEncoder();
    ~NvEncoder();

    void Init(const NV_ENC_DEVICE_TYPE deviceType, void *device, const uint32_t width, const uint32_t height);
    void CleanUp();

    void CreateBuffers(const uint32_t numBuffers);
    void DestroyBuffers();
    void RegisterInputResource(const uint32_t idx, CUarray input); 
    void EncodeFrame(const uint32_t imageIndex);


private:

    NV_ENCODE_API_FUNCTION_LIST m_nvenc;
    void LoadNvEncApi();
    void InitEncoder(const uint32_t width, const uint32_t height);
    void DestroyHWEncoder();
    void DestroyBitstreamBuffer();

    NV_ENC_REGISTERED_PTR RegisterResource(void *pBuffer, const NV_ENC_INPUT_RESOURCE_TYPE eResourceType,
        const uint32_t width, const uint32_t height, const NV_ENC_BUFFER_FORMAT bufferFormat, 
        const NV_ENC_BUFFER_USAGE bufferUsage);


    void *m_encoder = nullptr;
    std::vector<NV_ENC_OUTPUT_PTR> m_bitStreamOutputBufferVector;
    std::vector<void *> m_completionEventVector;

    std::vector<NV_ENC_REGISTERED_PTR> m_registeredInputResources;
    std::vector<NV_ENC_INPUT_PTR> m_mappedInputBuffers;
    uint32_t m_numEncoderBuffer = 0;
    NV_ENC_CONFIG m_encodeConfig;
    bool m_isEncoderInitialized;
    uint32_t m_width;
    uint32_t m_height;

};
