
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

void NvEncoder::Init(const NV_ENC_DEVICE_TYPE deviceType, void *device, const uint32_t width, const uint32_t height)
{
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

    InitEncoder(width, height);
}

//---------------------------------------------------------------------------------------------------------------------

void NvEncoder::CleanUp() {
    DestroyHWEncoder();
}

//---------------------------------------------------------------------------------------------------------------------
void NvEncoder::CreateBuffers(const uint32_t numBuffers) {
    m_inputBuffers.resize(numBuffers, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void NvEncoder::DestroyBuffers() {
    const uint32_t numBuffers = static_cast<uint32_t>(m_inputBuffers.size());
    for (uint32_t i = 0; i < numBuffers; ++i)
    {
        if (m_inputBuffers[i])
        {
            m_nvenc.nvEncUnmapInputResource(m_encoder, m_inputBuffers[i]);
        }
    }
    m_inputBuffers.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void NvEncoder::EncodeFrame(const uint32_t imageIndex) {

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

void NvEncoder::InitEncoder(const uint32_t width, const uint32_t height) {
    if (!m_encoder) {
        NVENC_THROW_ERROR("Encoder Initialization failed", NV_ENC_ERR_NO_ENCODE_DEVICE);
    }

    if (width <= 0 || height <= 0) {
        NVENC_THROW_ERROR("Invalid encoder width and height", NV_ENC_ERR_INVALID_PARAM);
    }

    //use default initialize params
    NV_ENC_INITIALIZE_PARAMS initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };

    initializeParams.version = NV_ENC_INITIALIZE_PARAMS_VER;
    initializeParams.encodeWidth = width;
    initializeParams.encodeHeight = height;
    initializeParams.darWidth = width;
    initializeParams.darHeight = height;
    initializeParams.encodeGUID = NV_ENC_CODEC_H264_GUID;
    initializeParams.presetGUID = NV_ENC_PRESET_LOW_LATENCY_HQ_GUID;
    initializeParams.frameRateNum = 45;
    initializeParams.frameRateDen = 1;
    initializeParams.enablePTD = 1;
    initializeParams.reportSliceOffsets = 0;
    initializeParams.enableSubFrameWrite = 0;
    initializeParams.maxEncodeWidth = width;
    initializeParams.maxEncodeHeight = height;
    initializeParams.enableEncodeAsync = 0; // Output to GPU

    //Always use preset config
    NV_ENC_PRESET_CONFIG presetConfig = { NV_ENC_PRESET_CONFIG_VER, { NV_ENC_CONFIG_VER } };
    m_nvenc.nvEncGetEncodePresetConfig(m_encoder, NV_ENC_CODEC_H264_GUID, NV_ENC_PRESET_DEFAULT_GUID, &presetConfig);
    memcpy(&m_encodeConfig, &presetConfig.presetCfg, sizeof(NV_ENC_CONFIG));
    m_encodeConfig.version = NV_ENC_CONFIG_VER;
    m_encodeConfig.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CONSTQP;
    m_encodeConfig.rcParams.constQP = { 28, 31, 25 };
    initializeParams.encodeConfig = &m_encodeConfig;

    NVENC_API_CALL(m_nvenc.nvEncInitializeEncoder(m_encoder, &initializeParams));

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
