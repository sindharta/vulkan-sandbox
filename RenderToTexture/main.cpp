#include <iostream> //std::exception, EXIT_SUCCESS, EXIT_FAILURE
#include "RenderToTextureApp.h"

int main() {
    RenderToTextureApp app;
    try {
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        app.CleanUp();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;

}