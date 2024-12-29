#pragma once

#include <vulkan/vulkan.h>

#include "pve_camera.hpp"
#include "pve_game_object.hpp"

namespace pve {

#define MAX_LIGHTS 10

struct PointLight {
    glm::vec4 position{};
    glm::vec4 color{};
};

struct GlobalUbo {
    glm::mat4 projection{1.f};
    glm::mat4 view{1.f};
    // using the inverseView matrix the camer's position can be calculated from the last column
    // also, we can transform a value from camera to world space
    glm::mat4 inverseView{1.f};
    glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .02f};
    PointLight pointLights[MAX_LIGHTS];
    int numLights;
};

struct FrameInfo {
    int frameIndex;
    float frameTime;
    VkCommandBuffer commandBuffer;
    PveCamera &camera;
    VkDescriptorSet globalDescriptorSet;
    PveGameObject::Map &gameObjects;
};
}  // namespace pve