#include <iostream> //std::exception, EXIT_SUCCESS, EXIT_FAILURE
#include "TriangleApp.h"

int main() {
    TriangleApp app;
    try {
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        app.CleanUp();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;

}