#include "src/app.hpp"

int main() {
    Application app(1280, 720, "Fluid");

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}