#include "systems/shadow_map_system.hpp"

#include "pve/pve_pipeline.hpp"

#define GLM_FORCE_RADIANS  // No matter what system i'm in, angles are in radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE  // Forces GLM to expect depth buffer values to range from 0 to 1 instead of -1 to 1 (the opengl standard)
#include <array>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <map>
#include <stdexcept>

namespace pve {

ShadowMapSystem::ShadowMapSystem(PveDevice& device) : pveDevice{device} {
    createShadowMapResources();
    createDescriptorSetLayout();
    createDescriptorPool();
    createSampler();
    createDescriptorSet();
}

ShadowMapSystem::~ShadowMapSystem() {
    vkDestroySampler(pveDevice.device(), shadowMapSampler, nullptr);
    vkDestroyDescriptorPool(pveDevice.device(), descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(pveDevice.device(), descriptorSetLayout, nullptr);
    vkDestroyImageView(pveDevice.device(), shadowMapImageView, nullptr);
    vkDestroyImage(pveDevice.device(), shadowMapImage, nullptr);
    vkFreeMemory(pveDevice.device(), shadowMapMemory, nullptr);
    vkDestroyFramebuffer(pveDevice.device(), shadowFramebuffer, nullptr);
    vkDestroyRenderPass(pveDevice.device(), shadowRenderPass, nullptr);
    vkDestroyPipelineLayout(pveDevice.device(), pipelineLayout, nullptr);
}

void ShadowMapSystem::createShadowMapResources() {
    createDepthResources();
    createRenderPass();
    createPipeline();
    createFramebuffer();
}

void ShadowMapSystem::createDepthResources() {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = SHADOW_MAP_SIZE;
    imageInfo.extent.height = SHADOW_MAP_SIZE;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = SHADOW_MAP_FORMAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage =
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    pveDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                  shadowMapImage, shadowMapMemory);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = shadowMapImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = SHADOW_MAP_FORMAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(pveDevice.device(), &viewInfo, nullptr, &shadowMapImageView) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow map image view!");
    }
}

void ShadowMapSystem::createRenderPass() {
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthReference{};
    depthReference.attachment = 0;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pDepthStencilAttachment = &depthReference;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &depthAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(pveDevice.device(), &renderPassInfo, nullptr,
                           &shadowRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow render pass!");
    }
}

void ShadowMapSystem::createPipeline() {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4) * 2;  // lightSpaceMatrix + modelMatrix

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(pveDevice.device(), &pipelineLayoutInfo, nullptr,
                               &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    PipelineConfigInfo pipelineConfig{};
    PvePipeline::defaultPipelineConfigInfo(pipelineConfig);

    // Modify config for shadow mapping
    pipelineConfig.renderPass = shadowRenderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    pipelineConfig.colorBlendInfo.attachmentCount = 0;

    shadowPipeline = std::make_unique<PvePipeline>(
        pveDevice, "shaders/compiled/shadow_map.vert.spv",
        "shaders/compiled/shadow_map.frag.spv",  // No fragment shader needed for depth-only pass
        pipelineConfig);
}

void ShadowMapSystem::createFramebuffer() {
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = shadowRenderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &shadowMapImageView;
    framebufferInfo.width = SHADOW_MAP_SIZE;
    framebufferInfo.height = SHADOW_MAP_SIZE;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(pveDevice.device(), &framebufferInfo, nullptr,
                            &shadowFramebuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow framebuffer!");
    }
}

void ShadowMapSystem::beginShadowPass(VkCommandBuffer commandBuffer) {
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = shadowRenderPass;
    renderPassInfo.framebuffer = shadowFramebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE};

    VkClearValue clearValue;
    clearValue.depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(SHADOW_MAP_SIZE);
    viewport.height = static_cast<float>(SHADOW_MAP_SIZE);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE};
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void ShadowMapSystem::recordShadowPass(FrameInfo& frameInfo, glm::mat4 lightSpaceMatrix) {
    beginShadowPass(frameInfo.commandBuffer);

    shadowPipeline->bind(frameInfo.commandBuffer);

    for (auto& kv : frameInfo.gameObjects) {
        auto& obj = kv.second;
        if (obj.model == nullptr) continue;

        // Push constants for shadow pass
        struct PushConstants {
            glm::mat4 lightSpaceMatrix;
            glm::mat4 modelMatrix;
        } push{};

        push.lightSpaceMatrix = lightSpaceMatrix;
        push.modelMatrix = obj.transform.mat4();

        vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &push);

        obj.model->bind(frameInfo.commandBuffer);
        obj.model->draw(frameInfo.commandBuffer);
    }

    endShadowPass(frameInfo.commandBuffer);
}

void ShadowMapSystem::endShadowPass(VkCommandBuffer commandBuffer) {
    vkCmdEndRenderPass(commandBuffer);
}

void ShadowMapSystem::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;  // matches shader binding
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;

    if (vkCreateDescriptorSetLayout(pveDevice.device(), &layoutInfo, nullptr,
                                    &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void ShadowMapSystem::createDescriptorPool() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(pveDevice.device(), &poolInfo, nullptr, &descriptorPool) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void ShadowMapSystem::createDescriptorSet() {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    if (vkAllocateDescriptorSets(pveDevice.device(), &allocInfo, &descriptorSet) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor set!");
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = shadowMapImageView;
    imageInfo.sampler = shadowMapSampler;  // You'll need to create this

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 1;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(pveDevice.device(), 1, &descriptorWrite, 0, nullptr);
}

void ShadowMapSystem::createSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_TRUE;
    samplerInfo.compareOp = VK_COMPARE_OP_LESS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(pveDevice.device(), &samplerInfo, nullptr, &shadowMapSampler) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow map sampler!");
    }
}

}  // namespace pve