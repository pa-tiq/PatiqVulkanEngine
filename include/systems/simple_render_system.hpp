#pragma once

#include <memory>
#include <vector>

#include "pve/pve_camera.hpp"
#include "pve/pve_device.hpp"
#include "pve/pve_frame_info.hpp"
#include "pve/pve_game_object.hpp"
#include "pve/pve_model.hpp"
#include "pve/pve_pipeline.hpp"

namespace pve {
class SimpleRenderSystem {
   public:
    SimpleRenderSystem(PveDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
    ~SimpleRenderSystem();

    SimpleRenderSystem(const SimpleRenderSystem &) = delete;
    SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

    // Renderer: swapchain, command buffers and draw frame
    void renderGameObjects(FrameInfo &frameInfo);

   private:
    void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);

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
