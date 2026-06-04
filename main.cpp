#include "src/app.hpp"

int main() {
    try {
        Application app(1280, 720, "Fluid");
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
