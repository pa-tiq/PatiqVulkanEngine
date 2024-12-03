#pragma once

#include "pve_device.hpp"

// libs
#define GLM_FORCE_RADIANS            // No matter what system i'm in, angles are in radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE  // Forces GLM to expect depth buffer values to range from 0 to 1 instead of -1 to 1 (the opengl standard)
#include <glm/glm.hpp>

// std
#include <memory>
#include <vector>

namespace pve {
// this class will take vertex data created by the CPU (or read in a file by the CPU)
// and allocate the memory and copy the data to the GPU so it can be rendered efficiently
class PveModel {
   public:
    struct Vertex {
        glm::vec3 position{};
        glm::vec3 color{};
        glm::vec3 normal{};
        glm::vec2 uv{};

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

        bool operator==(const Vertex &other) const {
            return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
        }
    };

    struct Builder {
        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};

        void loadModel(const std::string &filepath);
    };

    PveModel(PveDevice &device, const PveModel::Builder &builder);
    ~PveModel();

    PveModel(const PveModel &) = delete;
    PveModel &operator=(const PveModel &) = delete;

    static std::unique_ptr<PveModel> createModelFromFile(PveDevice &device, const std::string &filepath);

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