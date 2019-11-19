#pragma once

#include <exception>
#include <string>
#include <sstream>

#include "nvEncodeAPI.h"

class NvEncException : public std::exception {
public:
    NvEncException(const std::string& errorStr, const NVENCSTATUS errorCode)
        : m_errorString(errorStr), m_errorCode(errorCode) {}

    virtual ~NvEncException() throw() {}
    virtual const char* what() const throw() { return m_errorString.c_str(); }
    NVENCSTATUS  getErrorCode() const { return m_errorCode; }
    const std::string& getErrorString() const { return m_errorString; }
    static NvEncException makeNvEncException(const std::string& errorStr, const NVENCSTATUS errorCode,
        const std::string& functionName, const std::string& fileName, int lineNo);
private:
    std::string m_errorString;
    NVENCSTATUS m_errorCode;
};

//---------------------------------------------------------------------------------------------------------------------
inline NvEncException NvEncException::makeNvEncException(const std::string& errorStr, const NVENCSTATUS errorCode, const std::string& functionName,
    const std::string& fileName, int lineNo)
{
    std::ostringstream errorLog;
    errorLog << functionName << " : " << errorStr << " at " << fileName << ":" << lineNo << std::endl;
    NvEncException exception(errorLog.str(), errorCode);
    return exception;
}