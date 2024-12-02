#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "first_app.hpp"

int main() {
    pve::FirstApp app{};

    try {
        app.runGravityPhysics();
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}