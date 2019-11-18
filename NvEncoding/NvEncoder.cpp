
#include "NvEncoder.h"
#include "NvEncException.h"
#include <assert.h>

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

NvEncoder::NvEncoder() : m_encoder(nullptr), m_isEncoderInitialized(false),
    m_width(0), m_height(0)
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
    DestroyBuffers();
    DestroyHWEncoder();
    m_width = m_height = 0;

}

//---------------------------------------------------------------------------------------------------------------------
void NvEncoder::CreateBuffers(const uint32_t numBuffers) {
    m_registeredInputResources.resize(numBuffers, nullptr);
    m_mappedInputBuffers.resize(numBuffers, nullptr);
    m_bitStreamOutputBuffers.resize(numBuffers, nullptr);

    for (uint32_t i = 0; i < numBuffers; ++i)
    {
        NV_ENC_CREATE_BITSTREAM_BUFFER createBitstreamBuffer = { NV_ENC_CREATE_BITSTREAM_BUFFER_VER };
        NVENC_API_CALL(m_nvenc.nvEncCreateBitstreamBuffer(m_encoder, &createBitstreamBuffer));
        m_bitStreamOutputBuffers[i] = createBitstreamBuffer.bitstreamBuffer;
    }

}

//---------------------------------------------------------------------------------------------------------------------
void NvEncoder::DestroyBuffers() {
    const uint32_t numBuffers = static_cast<uint32_t>(m_mappedInputBuffers.size());
    for (uint32_t i = 0; i < numBuffers; ++i)
    {
        if (m_mappedInputBuffers[i]) {
            m_nvenc.nvEncUnmapInputResource(m_encoder, m_mappedInputBuffers[i]);
            m_mappedInputBuffers[i] = nullptr;
        }

        if (m_registeredInputResources[i]) {
            m_nvenc.nvEncUnregisterResource(m_encoder, m_registeredInputResources[i]);
            m_registeredInputResources[i] = nullptr;
        }
    }
    m_mappedInputBuffers.clear();
    m_registeredInputResources.clear();

    DestroyBitstreamBuffer();
}

//---------------------------------------------------------------------------------------------------------------------
void NvEncoder::RegisterInputResource(const uint32_t idx, CUarray input) {
    assert(idx<m_registeredInputResources.size());
    assert(nullptr == m_registeredInputResources[idx]); //we need to unregister this if we want to suppor reassigning

    const NV_ENC_INPUT_RESOURCE_TYPE resourceType = NV_ENC_INPUT_RESOURCE_TYPE_CUDAARRAY;
    const NV_ENC_BUFFER_FORMAT bufferFormat = NV_ENC_BUFFER_FORMAT_ARGB;

    NV_ENC_REGISTERED_PTR registeredPtr = RegisterResource(input, resourceType, m_width, m_height, 
        bufferFormat, NV_ENC_INPUT_IMAGE);

    m_registeredInputResources[idx] = registeredPtr;
}

//---------------------------------------------------------------------------------------------------------------------
void NvEncoder::EncodeFrame(const uint32_t imageIndex) {

    if (!m_isEncoderInitialized) {
        NVENC_THROW_ERROR("Encoder device not found", NV_ENC_ERR_NO_ENCODE_DEVICE);
    }

    //Map resources
    NV_ENC_MAP_INPUT_RESOURCE mapInputResource = { NV_ENC_MAP_INPUT_RESOURCE_VER };
    mapInputResource.registeredResource = m_registeredInputResources[imageIndex];
    NVENC_API_CALL(m_nvenc.nvEncMapInputResource(m_encoder, &mapInputResource));
    m_mappedInputBuffers[imageIndex] = mapInputResource.mappedResource;


    const NVENCSTATUS nvStatus = DoEncode(m_mappedInputBuffers[imageIndex], m_bitStreamOutputBuffers[imageIndex]);

    if (NV_ENC_SUCCESS != nvStatus  && NV_ENC_ERR_NEED_MORE_INPUT != nvStatus) {
        NVENC_THROW_ERROR("nvEncEncodePicture API failed", nvStatus);
    }

    //Unmap
    NVENC_API_CALL(m_nvenc.nvEncUnmapInputResource(m_encoder, m_mappedInputBuffers[imageIndex]));

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
    m_width = width;
    m_height = height;

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
//    initializeParams.enableOutputInVidmem = true;

    //Always use preset config
    NV_ENC_PRESET_CONFIG presetConfig = { NV_ENC_PRESET_CONFIG_VER, { NV_ENC_CONFIG_VER } };
    m_nvenc.nvEncGetEncodePresetConfig(m_encoder, NV_ENC_CODEC_H264_GUID, NV_ENC_PRESET_DEFAULT_GUID, &presetConfig);
    memcpy(&m_encodeConfig, &presetConfig.presetCfg, sizeof(NV_ENC_CONFIG));
    m_encodeConfig.version = NV_ENC_CONFIG_VER;
    m_encodeConfig.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CONSTQP;
    m_encodeConfig.rcParams.constQP = { 28, 31, 25 };
    initializeParams.encodeConfig = &m_encodeConfig;

    NVENC_API_CALL(m_nvenc.nvEncInitializeEncoder(m_encoder, &initializeParams));
    m_isEncoderInitialized = true;
}


//---------------------------------------------------------------------------------------------------------------------

void NvEncoder::DestroyHWEncoder() {
    if (!m_encoder)     {
        return;
    }

    DestroyBitstreamBuffer();

    m_nvenc.nvEncDestroyEncoder(m_encoder);
    m_encoder = nullptr;
}

//---------------------------------------------------------------------------------------------------------------------

void NvEncoder::DestroyBitstreamBuffer() {
    const uint32_t num = static_cast<uint32_t>(m_bitStreamOutputBuffers.size());
    for (uint32_t i = 0; i < num; i++){
        if (m_bitStreamOutputBuffers[i]) {
            m_nvenc.nvEncDestroyBitstreamBuffer(m_encoder, m_bitStreamOutputBuffers[i]);
        }
    }

    m_bitStreamOutputBuffers.clear();
}

//---------------------------------------------------------------------------------------------------------------------

NV_ENC_REGISTERED_PTR NvEncoder::RegisterResource(void *pBuffer, const NV_ENC_INPUT_RESOURCE_TYPE eResourceType,
    const uint32_t width, const uint32_t height, const NV_ENC_BUFFER_FORMAT bufferFormat, 
    const NV_ENC_BUFFER_USAGE bufferUsage)
{
    NV_ENC_REGISTER_RESOURCE registerResource = { NV_ENC_REGISTER_RESOURCE_VER };
    registerResource.resourceType = eResourceType;
    registerResource.resourceToRegister = pBuffer;
    registerResource.width = width;
    registerResource.height = height;
    //registerResource.pitch = pitch;
    registerResource.bufferFormat = bufferFormat;
    registerResource.bufferUsage = bufferUsage;
    NVENC_API_CALL(m_nvenc.nvEncRegisterResource(m_encoder, &registerResource));

    return registerResource.registeredResource;
}

//---------------------------------------------------------------------------------------------------------------------

NVENCSTATUS NvEncoder::DoEncode(NV_ENC_INPUT_PTR inputBuffer, NV_ENC_OUTPUT_PTR outputBuffer) 
{
    NV_ENC_PIC_PARAMS picParams = {};
    picParams.version = NV_ENC_PIC_PARAMS_VER;
    picParams.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
    picParams.inputBuffer = inputBuffer;
    picParams.bufferFmt = NV_ENC_BUFFER_FORMAT_ARGB;
    picParams.inputWidth = m_width;
    picParams.inputHeight = m_height;
    picParams.outputBitstream = outputBuffer;
    const NVENCSTATUS nvStatus = m_nvenc.nvEncEncodePicture(m_encoder, &picParams);

    return nvStatus; 

}

