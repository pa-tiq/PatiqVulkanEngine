#include "simple_render_system.hpp"

#define GLM_FORCE_RADIANS  // No matter what system i'm in, angles are in radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE  // Forces GLM to expect depth buffer values to range from 0 to 1 instead of -1 to 1 (the opengl standard)
#include <array>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <stdexcept>

namespace pve {

// temporary
struct SimplePushConstantData {
    glm::mat2 transform{1.f};  // initialized as an identity matrix
    glm::vec2 offset;
    alignas(16) glm::vec3 color;  // alignas: tutorial 9 at 09:00
};

static glm::vec3 red = {1.0f, 0.0f, 0.0f};
static glm::vec3 green = {0.0f, 1.0f, 0.0f};
static glm::vec3 blue = {0.0f, 0.0f, 1.0f};

SimpleRenderSystem::SimpleRenderSystem(PveDevice &device, VkRenderPass renderPass)
    : pveDevice{device} {
    createPipelineLayout();
    createPipeline(renderPass);
}

SimpleRenderSystem::~SimpleRenderSystem() {
    vkDestroyPipelineLayout(pveDevice.device(), pipelineLayout, nullptr);
}

void SimpleRenderSystem::createPipelineLayout() {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SimplePushConstantData);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
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
        std::make_unique<PvePipeline>(pveDevice, "shaders/simple_shader.vert.spv",
                                      "shaders/simple_shader.frag.spv", pipelineConfig);
}

void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer,
                                           std::vector<PveGameObject> &gameObjects) {
    // update
    int i = 0;
    for (auto &obj : gameObjects) {
        i += 1;
        obj.transform2d.rotation = glm::mod<float>(obj.transform2d.rotation + 0.001f * i,
                                                   2.f * glm::pi<float>());
    }
    // render
    pvePipeline->bind(commandBuffer);
    for (auto &obj : gameObjects) {
        SimplePushConstantData push{};
        push.offset = obj.transform2d.translation;
        push.color = obj.color;
        push.transform = obj.transform2d.mat2();
        vkCmdPushConstants(commandBuffer, pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(SimplePushConstantData), &push);
        obj.model->bind(commandBuffer);
        obj.model->draw(commandBuffer);
    }
}

}  // namespace pve