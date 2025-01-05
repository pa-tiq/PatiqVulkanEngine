#pragma once

#include "pve/pve_descriptors.hpp"
#include "pve/pve_device.hpp"
#include "pve/pve_frame_info.hpp"
#include "pve/pve_pipeline.hpp"

namespace pve {

class ShadowMapSystem {
   public:
    static constexpr VkFormat SHADOW_MAP_FORMAT = VK_FORMAT_D32_SFLOAT;
    static constexpr uint32_t SHADOW_MAP_SIZE = 2048;  // Adjust resolution as needed

    ShadowMapSystem(PveDevice& device);
    ~ShadowMapSystem();

    VkImageView getShadowMapView() const { return shadowMapImageView; }
    VkSampler getShadowMapSampler() const { return shadowMapSampler; }

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

    VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImage shadowMapImage = VK_NULL_HANDLE;
    VkDeviceMemory shadowMapMemory = VK_NULL_HANDLE;
    VkImageView shadowMapImageView = VK_NULL_HANDLE;
    VkSampler shadowMapSampler = VK_NULL_HANDLE;

    VkFramebuffer shadowFramebuffer;
    VkRenderPass shadowRenderPass;
    std::unique_ptr<PvePipeline> shadowPipeline;
    VkPipelineLayout pipelineLayout;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;

    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSet();
    void createSampler();
};

}  // namespace pve