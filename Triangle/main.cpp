#include <iostream> //std::exception, EXIT_SUCCESS, EXIT_FAILURE
#include "TriangleApp.h"

int main() {
    TriangleApp app;
    try {
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;

    ///
    //uint32_t extensionCount = 0;
    //vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    //std::cout << extensionCount << " extensions supported" << std::endl;
    //glm::mat4 matrix;
    //glm::vec4 vec;
    //auto test = matrix * vec;

}