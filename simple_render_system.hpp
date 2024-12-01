#pragma once

#include <memory>
#include <vector>

#include "pve_device.hpp"
#include "pve_game_object.hpp"
#include "pve_model.hpp"
#include "pve_pipeline.hpp"

namespace pve {
class SimpleRenderSystem {
   public:
    SimpleRenderSystem(PveDevice &device, VkRenderPass renderPass);
    ~SimpleRenderSystem();

    SimpleRenderSystem(const SimpleRenderSystem &) = delete;
    SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

    // Renderer: swapchain, command buffers and draw frame
    void renderGameObjects(VkCommandBuffer commandBuffer,
                           std::vector<PveGameObject> &gameObjects);

   private:
    void createPipelineLayout();

    // The renderPass will be used just to create the pipeline, we're not going to store it
    // because the render system's lifecycle is not tied to the renderPass
    void createPipeline(VkRenderPass renderPass);

    PveDevice &pveDevice;
    // a smart pointer simulates a pointer but with the addition of automatic
    // memory management
    std::unique_ptr<PvePipeline> pvePipeline;
    VkPipelineLayout pipelineLayout;
};
}  // namespace pve
