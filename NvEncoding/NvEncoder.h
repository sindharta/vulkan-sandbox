#pragma once
#include "nvEncodeAPI.h"
#include <vector>


class NvEncoder {
public:
    NvEncoder();
    ~NvEncoder();

    void Init(const NV_ENC_DEVICE_TYPE deviceType, void *device, const uint32_t width, const uint32_t height);
    void CleanUp();


private:

    NV_ENCODE_API_FUNCTION_LIST m_nvenc;
    void LoadNvEncApi();
    void InitEncoder(const uint32_t width, const uint32_t height);
    void DestroyHWEncoder();
    void DestroyBitstreamBuffer();



    void *m_encoder = nullptr;
    std::vector<NV_ENC_OUTPUT_PTR> m_bitStreamOutputBufferVector;
    std::vector<void *> m_completionEventVector;
    uint32_t m_numEncoderBuffer = 0;
    NV_ENC_CONFIG m_encodeConfig;

};
