#pragma once

#include <memory>
#include <vector>

#include "pve_device.hpp"
#include "pve_game_object.hpp"
#include "pve_model.hpp"
#include "pve_pipeline.hpp"
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

   private:
    void sierpinski(std::vector<PveModel::Vertex> &vertices, int depth, glm::vec2 left,
                    glm::vec2 right, glm::vec2 top);
    void sierpinskiTriangle();
    void spinningTriangle();
    void crazyTriangles();

    void loadGameObjects();
    void createPipelineLayout();
    void createPipeline();

    // Renderer: swapchain, command buffers and draw frame
    void renderGameObjects(VkCommandBuffer commandBuffer);

    PveWindow pveWindow{WIDTH, HEIGHT, "Hello Vulkan!"};
    PveDevice pveDevice{pveWindow};
    PveRenderer pveRenderer{pveWindow, pveDevice};
    // a smart pointer simulates a pointer but with the addition of automatic
    // memory management
    std::unique_ptr<PvePipeline> pvePipeline;
    VkPipelineLayout pipelineLayout;
    std::vector<PveGameObject> gameObjects;
};
}  // namespace pve
