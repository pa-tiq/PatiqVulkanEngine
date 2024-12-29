#include "systems/point_light_system.hpp"

#define GLM_FORCE_RADIANS  // No matter what system i'm in, angles are in radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE  // Forces GLM to expect depth buffer values to range from 0 to 1 instead of -1 to 1 (the opengl standard)
#include <array>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <map>
#include <stdexcept>

namespace pve {

struct PointLightPushConstants {
    glm::vec4 position{};
    glm::vec4 color{};
    float radius;
};

PointLightSystem::PointLightSystem(PveDevice &device, VkRenderPass renderPass,
                                   VkDescriptorSetLayout globalSetLayout)
    : pveDevice{device} {
    createPipelineLayout(globalSetLayout);
    createPipeline(renderPass);
}

PointLightSystem::~PointLightSystem() {
    vkDestroyPipelineLayout(pveDevice.device(), pipelineLayout, nullptr);
}

void PointLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PointLightPushConstants);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount =
        static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(pveDevice.device(), &pipelineLayoutInfo, nullptr,
                               &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }
}

void PointLightSystem::createPipeline(VkRenderPass renderPass) {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    PvePipeline::defaultPipelineConfigInfo(pipelineConfig);
    PvePipeline::enableAlphaBlending(pipelineConfig);

    // Point light system doesn't need vertex data
    pipelineConfig.attributeDescriptions.clear();
    pipelineConfig.bindingDescriptions.clear();

    // a render pass describes the structure and format of our frame buffer objects and their attachments
    // it's a blueprint that tells a graphics pipeline object what layout to expect from the output frame buffer
    // when it's time to actually render, the graphics pipeline is already prepared to output to our frame buffer,
    // as long as the passed frame buffer object is setup in a way that is compatible with what we specified in the render pass.
    // multiple subpasses can be grouped together into a single render pass
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    pvePipeline = std::make_unique<PvePipeline>(
        pveDevice, "shaders/compiled/point_light.vert.spv",
        "shaders/compiled/point_light.frag.spv", pipelineConfig);
}

void PointLightSystem::update(FrameInfo &frameInfo, GlobalUbo &ubo) {
    auto rotateLight =
        glm::rotate(glm::mat4(1.f), frameInfo.frameTime, glm::vec3(0.f, -1.f, 0.f));
    int lightIndex = 0;
    for (auto &kv : frameInfo.gameObjects) {
        auto &obj = kv.second;
        if (obj.pointLight == nullptr) continue;
        assert(lightIndex < MAX_LIGHTS && "Point light index out of bounds");

        // update light position
        // obj.transform.translation =
        //     glm::vec3(rotateLight * glm::vec4(obj.transform.translation, 1.f));

        // copy light data to ubo
        ubo.pointLights[lightIndex].position = glm::vec4(obj.transform.translation, 1.f);
        ubo.pointLights[lightIndex].color =
            glm::vec4(obj.color, obj.pointLight->lightIntensity);
        lightIndex++;
    }
    ubo.numLights = lightIndex;
}

void PointLightSystem::render(FrameInfo &frameInfo) {
    // sort lights by distance to camera to make transparency work
    std::map<float, PveGameObject::id_t> sorted;
    for (auto &kv : frameInfo.gameObjects) {
        auto &obj = kv.second;
        if (obj.pointLight == nullptr) continue;
        auto offset = frameInfo.camera.getPosition() - obj.transform.translation;
        float disSquared = glm::dot(offset, offset);
        sorted[disSquared] = obj.getId();
    }

    pvePipeline->bind(frameInfo.commandBuffer);
    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, &frameInfo.globalDescriptorSet, 0,
                            nullptr);
    // iterate through sorted lights in reverse order since we render from back to front
    for (auto it = sorted.rbegin(); it != sorted.rend(); it++) {
        auto &obj = frameInfo.gameObjects.at(it->second);

        PointLightPushConstants pushConstantData{};
        pushConstantData.position = glm::vec4(obj.transform.translation, 1.f);
        pushConstantData.color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
        pushConstantData.radius = obj.transform.scale.x;

        vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(PointLightPushConstants), &pushConstantData);

        vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
    }
}

}  // namespace pve
