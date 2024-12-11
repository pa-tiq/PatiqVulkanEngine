#include "systems/simple_render_system.hpp"

#define GLM_FORCE_RADIANS            // No matter what system i'm in, angles are in radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE  // Forces GLM to expect depth buffer values to range from 0 to 1 instead of -1 to 1 (the opengl standard)
#include <array>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <stdexcept>

namespace pve {

struct SimplePushConstantData {
    glm::mat4 modelMatrix{1.f};   // initialized as an identity matrix
    glm::mat4 normalMatrix{1.f};  // initialized as an identity matrix
};

SimpleRenderSystem::SimpleRenderSystem(PveDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
    : pveDevice{device} {
    createPipelineLayout(globalSetLayout);
    createPipeline(renderPass);
}

SimpleRenderSystem::~SimpleRenderSystem() {
    vkDestroyPipelineLayout(pveDevice.device(), pipelineLayout, nullptr);
}

void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SimplePushConstantData);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(pveDevice.device(), &pipelineLayoutInfo, nullptr,
                               &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }
}

void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    PvePipeline::defaultPipelineConfigInfo(pipelineConfig);
    // a render pass describes the structure and format of our frame buffer objects and their attachments
    // it's a blueprint that tells a graphics pipeline object what layout to expect from the output frame buffer
    // when it's time to actually render, the graphics pipeline is already prepared to output to our frame buffer,
    // as long as the passed frame buffer object is setup in a way that is compatible with what we specified in the render pass.
    // multiple subpasses can be grouped together into a single render pass
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    pvePipeline =
        std::make_unique<PvePipeline>(pveDevice, "shaders/compiled/simple_shader.vert.spv",
                                      "shaders/compiled/simple_shader.frag.spv", pipelineConfig);
}

void SimpleRenderSystem::renderGameObjects(FrameInfo &frameInfo) {
    pvePipeline->bind(frameInfo.commandBuffer);
    vkCmdBindDescriptorSets(
        frameInfo.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout,
        0,
        1,
        &frameInfo.globalDescriptorSet,
        0,
        nullptr);

    for (auto &keyvalue : frameInfo.gameObjects) {
        auto &obj = keyvalue.second;
        if (obj.model == nullptr) continue;
        if (obj.name == "cube") {
            obj.transform.rotation.y = glm::mod(obj.transform.rotation.y + 0.001f,
                                                glm::two_pi<float>());
            obj.transform.rotation.x = glm::mod(obj.transform.rotation.x + 0.005f,
                                                glm::two_pi<float>());
        }

        SimplePushConstantData push{};
        push.modelMatrix = obj.transform.mat4();
        push.normalMatrix = obj.transform.normalMatrix();

        vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(SimplePushConstantData), &push);
        obj.model->bind(frameInfo.commandBuffer);
        obj.model->draw(frameInfo.commandBuffer);
    }
}

}  // namespace pve
