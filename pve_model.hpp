#pragma once

#include "pve_device.hpp"

#define GLM_FORCE_RADIANS // No matter what system i'm in, angles are in radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Forces GLM to expect depth buffer values to range from 0 to 1
#include <glm/glm.hpp>

namespace pve
{
    // this class will take vertex data created by the CPU (or read in a file by the CPU)
    // and allocate the memory and copy the data to the GPU so it can be rendered efficiently
    class PveModel
    {

    public:

        struct Vertex {
            glm::vec2 position;
            glm::vec3 color;

            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
        };

        PveModel(PveDevice &device, const std::vector<Vertex> &vertices);
        ~PveModel();

        PveModel(const PveModel &) = delete;
        PveModel &operator=(const PveModel &) = delete;

        void bind(VkCommandBuffer commandBuffer);
        void draw(VkCommandBuffer commandBuffer);

    private:
        void createVertexBuffer(const std::vector<Vertex> &vertices);
        PveDevice &pveDevice;
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory; // memory is not automatically assigned to the buffer
        uint32_t vertexCount;
    };
}