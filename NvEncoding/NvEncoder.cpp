
#include "NvEncoder.h"
#include "NvEncException.h"

//NVEncoder Macros
#define NVENC_API_CALL( nvencAPI )                                                                                 \
    do                                                                                                             \
    {                                                                                                              \
        NVENCSTATUS errorCode = nvencAPI;                                                                          \
        if( errorCode != NV_ENC_SUCCESS)                                                                           \
        {                                                                                                          \
            std::ostringstream errorLog;                                                                           \
            errorLog << #nvencAPI << " returned error " << errorCode;                                              \
            throw NvEncException::makeNvEncException(errorLog.str(), errorCode, __FUNCTION__, __FILE__, __LINE__); \
        }                                                                                                          \
    } while (0)

#define NVENC_THROW_ERROR( errorStr, errorCode )                                                         \
    do                                                                                                   \
    {                                                                                                    \
        throw NvEncException::makeNvEncException(errorStr, errorCode, __FUNCTION__, __FILE__, __LINE__); \
    } while (0)

//---------------------------------------------------------------------------------------------------------------------

NvEncoder::NvEncoder() : m_encoder(nullptr), m_numEncoderBuffer(0)
{

}

//---------------------------------------------------------------------------------------------------------------------

NvEncoder::~NvEncoder() {
     
}

//---------------------------------------------------------------------------------------------------------------------

void NvEncoder::Init(NV_ENC_DEVICE_TYPE deviceType, void *device) {

    LoadNvEncApi();

    if (!m_nvenc.nvEncOpenEncodeSession) {
m_numEncoderBuffer = 0;
        NVENC_THROW_ERROR("EncodeAPI not found", NV_ENC_ERR_NO_ENCODE_DEVICE);
    }

    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS encodeSessionExParams = { NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER };
    encodeSessionExParams.device = device;
    encodeSessionExParams.deviceType = deviceType;
    encodeSessionExParams.apiVersion = NVENCAPI_VERSION;
    NVENC_API_CALL(m_nvenc.nvEncOpenEncodeSessionEx(&encodeSessionExParams, &m_encoder));
}

//---------------------------------------------------------------------------------------------------------------------

void NvEncoder::CleanUp() {
    DestroyHWEncoder();
}

//---------------------------------------------------------------------------------------------------------------------

void NvEncoder::LoadNvEncApi() {

    uint32_t version = 0;
    uint32_t currentVersion = (NVENCAPI_MAJOR_VERSION << 4) | NVENCAPI_MINOR_VERSION;
    NVENC_API_CALL(NvEncodeAPIGetMaxSupportedVersion(&version));
    if (currentVersion > version) {
        NVENC_THROW_ERROR("Current Driver Version does not support this NvEncodeAPI version, please upgrade driver", NV_ENC_ERR_INVALID_VERSION);
    }
    
    m_nvenc = { NV_ENCODE_API_FUNCTION_LIST_VER };
    NVENC_API_CALL(NvEncodeAPICreateInstance(&m_nvenc));
}

//---------------------------------------------------------------------------------------------------------------------

void NvEncoder::DestroyHWEncoder() {
    if (!m_encoder)     {
        return;
    }

#if defined(_WIN32)
    const uint32_t num = static_cast<uint32_t>(m_completionEventVector.size());
    for (uint32_t i = 0; i < num; i++)     {
        if (m_completionEventVector[i]) {
            NV_ENC_EVENT_PARAMS eventParams = { NV_ENC_EVENT_PARAMS_VER };
            eventParams.completionEvent = m_completionEventVector[i];
            m_nvenc.nvEncUnregisterAsyncEvent(m_encoder, &eventParams);
            CloseHandle(m_completionEventVector[i]);
        }
    }
    m_completionEventVector.clear();
#endif

    DestroyBitstreamBuffer();

    m_nvenc.nvEncDestroyEncoder(m_encoder);
    m_encoder = nullptr;
}


//---------------------------------------------------------------------------------------------------------------------

void NvEncoder::DestroyBitstreamBuffer() {
    const uint32_t num = static_cast<uint32_t>(m_bitStreamOutputBufferVector.size());
    for (uint32_t i = 0; i < num; i++){
        if (m_bitStreamOutputBufferVector[i]) {
            m_nvenc.nvEncDestroyBitstreamBuffer(m_encoder, m_bitStreamOutputBufferVector[i]);
        }
    }

    m_bitStreamOutputBufferVector.clear();
}
