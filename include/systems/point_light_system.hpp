#pragma once

#include <memory>
#include <vector>

#include "pve/pve_camera.hpp"
#include "pve/pve_device.hpp"
#include "pve/pve_frame_info.hpp"
#include "pve/pve_game_object.hpp"
#include "pve/pve_model.hpp"
#include "pve/pve_pipeline.hpp"
#include "systems/shadow_map_system.hpp"

namespace pve {
class PointLightSystem {
   public:
    PointLightSystem(PveDevice &device, VkRenderPass renderPass,
                     VkDescriptorSetLayout globalSetLayout);
    ~PointLightSystem();

    PointLightSystem(const PointLightSystem &) = delete;
    PointLightSystem &operator=(const PointLightSystem &) = delete;

    void update(FrameInfo &frameInfo, GlobalUbo &ubo);
    // Renderer: swapchain, command buffers and draw frame
    void render(FrameInfo &frameInfo);
    void updateShadowMap(FrameInfo &frameInfo);

    void setMainLightPosition(glm::vec3 position) { mainLightPos = position; }
    void setMainLightTarget(glm::vec3 target) { mainLightTarget = target; }
    void setLightOrthoSize(float size) { lightOrthoSize = size; }

   private:
    glm::vec3 mainLightPos = {-2.0f, 4.0f, -1.0f};
    glm::vec3 mainLightTarget = {0.0f, 0.0f, 0.0f};
    float lightOrthoSize = 10.0f;

    void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);

    // The renderPass will be used just to create the pipeline, we're not going to store it
    // because the render system's lifecycle is not tied to the renderPass
    void createPipeline(VkRenderPass renderPass);

    PveDevice &pveDevice;
    // a smart pointer simulates a pointer but with the addition of automatic
    // memory management
    std::unique_ptr<PvePipeline> pvePipeline;
    VkPipelineLayout pipelineLayout;

    std::unique_ptr<ShadowMapSystem> shadowMapSystem;
};
}  // namespace pve
