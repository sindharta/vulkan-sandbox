#pragma once
#include <vector>
#include <fstream> //ifstream


class FileUtility {
    public:
        static void ReadFileInto(const std::string& filename, std::vector<char>* buffer);
};