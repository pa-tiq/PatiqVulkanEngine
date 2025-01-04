#pragma once

#include "pve/pve_descriptors.hpp"
#include "pve/pve_device.hpp"
#include "pve/pve_frame_info.hpp"
#include "pve/pve_pipeline.hpp"

namespace pve {

class ShadowMapSystem {
   public:
    static constexpr uint32_t SHADOW_MAP_SIZE = 2048;  // Adjust resolution as needed

    ShadowMapSystem(PveDevice& device);
    ~ShadowMapSystem();

    void createShadowMapResources();
    void beginShadowPass(VkCommandBuffer commandBuffer);
    void recordShadowPass(FrameInfo& frameInfo, glm::mat4 lightSpaceMatrix);
    void endShadowPass(VkCommandBuffer commandBuffer);

   private:
    void createDepthResources();
    void createRenderPass();
    void createFramebuffer();
    void createPipeline();

    PveDevice& pveDevice;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    VkFramebuffer shadowFramebuffer;
    VkRenderPass shadowRenderPass;
    std::unique_ptr<PvePipeline> shadowPipeline;
    VkPipelineLayout pipelineLayout;
};

}  // namespace pve