#pragma once

#include <memory>
#include <vector>

#include "pve_device.hpp"
#include "pve_game_object.hpp"
#include "pve_renderer.hpp"
#include "pve_window.hpp"

namespace pve {
class FirstApp {
   public:
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 600;

    FirstApp();
    ~FirstApp();

    FirstApp(const FirstApp &) = delete;
    FirstApp &operator=(const FirstApp &) = delete;

    void run();
    void runGravityPhisics();

   private:
    void sierpinski(std::vector<PveModel::Vertex> &vertices, int depth, glm::vec2 left,
                    glm::vec2 right, glm::vec2 top);
    void sierpinskiTriangle();

    void spinningTriangle();
    void crazyTriangles();

    std::unique_ptr<PveModel> createSquareModel(PveDevice &device, glm::vec2 offset);
    std::unique_ptr<PveModel> createCircleModel(PveDevice &device, unsigned int numSides);
    void GravityPhisics();

    void loadGameObjects();

    PveWindow pveWindow{WIDTH, HEIGHT, "Hello Vulkan!"};
    PveDevice pveDevice{pveWindow};
    PveRenderer pveRenderer{pveWindow, pveDevice};

    std::vector<PveGameObject> gameObjects;
};
}  // namespace pve
