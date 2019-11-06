#include "FileUtility.h"

void FileUtility::ReadFileInto(const std::string& filename, std::vector<char>* buffer) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    //We are at the end. So just get the current pos to know the size
    const size_t fileSize = (size_t) file.tellg();
    buffer->resize(fileSize);
    file.seekg(0);
    file.read(buffer->data(), fileSize);
    file.close();
}
