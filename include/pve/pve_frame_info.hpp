#pragma once

#include <vulkan/vulkan.h>

#include "pve_camera.hpp"
#include "pve_game_object.hpp"

namespace pve {
struct FrameInfo {
    int frameIndex;
    float frameTime;
    VkCommandBuffer commandBuffer;
    PveCamera &camera;
    VkDescriptorSet globalDescriptorSet;
    PveGameObject::Map &gameObjects;
};
}  // namespace pve