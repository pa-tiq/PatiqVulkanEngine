#pragma once

#include "pve_device.hpp"

#define GLM_FORCE_RADIANS            // No matter what system i'm in, angles are in radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE  // Forces GLM to expect depth buffer values to range from 0 to 1 instead of -1 to 1 (the opengl standard)
#include <glm/glm.hpp>
#include <vector>

namespace pve {
// this class will take vertex data created by the CPU (or read in a file by the CPU)
// and allocate the memory and copy the data to the GPU so it can be rendered efficiently
class PveModel {
   public:
    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    struct Builder {
        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};
    };

    PveModel(PveDevice &device, const PveModel::Builder &builder);
    ~PveModel();

    PveModel(const PveModel &) = delete;
    PveModel &operator=(const PveModel &) = delete;

    void bind(VkCommandBuffer commandBuffer);
    void draw(VkCommandBuffer commandBuffer);

    // the buffer and its assigned memory are two separate objects
    // memory is not automatically assigned to the buffer
    // the programmer controls memory management
   private:
    void createVertexBuffers(const std::vector<Vertex> &vertices);
    void createIndexBuffers(const std::vector<uint32_t> &indices);

    PveDevice &pveDevice;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    uint32_t vertexCount;

    bool hasIndexBuffer = false;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    uint32_t indexCount;
};
}  // namespace pve