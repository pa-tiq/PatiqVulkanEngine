#include "pve_model.hpp"

// libs
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// std
#include <cassert>
#include <cstring>
#include <iostream>

namespace pve {
PveModel::PveModel(PveDevice &device, const PveModel::Builder &builder)
    : pveDevice{device} {
    createVertexBuffers(builder.vertices);
    createIndexBuffers(builder.indices);
}

PveModel::~PveModel() {
    vkDestroyBuffer(pveDevice.device(), vertexBuffer, nullptr);
    vkFreeMemory(pveDevice.device(), vertexBufferMemory, nullptr);
    if (hasIndexBuffer) {
        vkDestroyBuffer(pveDevice.device(), indexBuffer, nullptr);
        vkFreeMemory(pveDevice.device(), indexBufferMemory, nullptr);
    }
}

std::unique_ptr<PveModel> PveModel::createModelFromFile(PveDevice &device, const std::string &filepath) {
    Builder builder{};
    builder.loadModel(filepath);
    std::cout << "Vertex count: " << builder.vertices.size() << "\n";
    return std::make_unique<PveModel>(device, builder);
}

void PveModel::createVertexBuffers(const std::vector<Vertex> &vertices) {
    vertexCount = static_cast<uint32_t>(vertices.size());
    assert(vertexCount >= 3 && "Vertex count must be at least 3");
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    pveDevice.createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  // Buffer will be used as the source location for a memory transfer operation
        // HOST = CPU, Device = GPU.
        // VISIBLE tells that allocated memory will be accessible from the host
        // this is necessary for the host to be able to write to the device's memory
        // COHERENT keeps the host and device memory regions consistent with each other
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);

    void *data;

    // create a region of HOST memory mapped to the DEVICE memory and sets
    // "data" to point to the beginning of the mapped memory range
    vkMapMemory(pveDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);

    // take the vertices data and copy it to the HOST mapped memory region
    // because of the COHERENT bit, the HOST memory will automatically be flushed to update the DEVICE memory
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(pveDevice.device(), stagingBufferMemory);

    pveDevice.createBuffer(
        bufferSize,
        // Buffer will be used to hold vertex input data or as as the destination location for a memory transfer operation
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vertexBuffer, vertexBufferMemory);

    pveDevice.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(pveDevice.device(), stagingBuffer, nullptr);
    vkFreeMemory(pveDevice.device(), stagingBufferMemory, nullptr);
}

void PveModel::createIndexBuffers(const std::vector<uint32_t> &indices) {
    indexCount = static_cast<uint32_t>(indices.size());
    hasIndexBuffer = indexCount > 0;
    if (!hasIndexBuffer) {
        return;
    }
    VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    pveDevice.createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);

    void *data;
    vkMapMemory(pveDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(pveDevice.device(), stagingBufferMemory);

    pveDevice.createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        indexBuffer, indexBufferMemory);

    pveDevice.copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(pveDevice.device(), stagingBuffer, nullptr);
    vkFreeMemory(pveDevice.device(), stagingBufferMemory, nullptr);
}

void PveModel::draw(VkCommandBuffer commandBuffer) {
    if (hasIndexBuffer) {
        vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
    } else {
        vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
    }
}

void PveModel::bind(VkCommandBuffer commandBuffer) {
    VkBuffer buffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    // record to commandBuffer to bind one vertexBuffer starting at binding 0 with offset of 0 into the buffer
    // when we want to add multiple bindings, just add additional elements to the arrays
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
    if (hasIndexBuffer) {
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    }
}

std::vector<VkVertexInputBindingDescription> PveModel::Vertex::getBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(Vertex);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // this binding description corresponds to a single vertex buffer.
    // it will occupy the first binding and index 0.
    return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription>
PveModel::Vertex::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    return attributeDescriptions;
}

void PveModel::Builder::loadModel(const std::string &filepath) {
    tinyobj::attrib_t attrib;              // position, color, normal, texture coordinate data
    std::vector<tinyobj::shape_t> shapes;  // index values for each face element
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
        throw std::runtime_error(warn + err);
    }

    vertices.clear();
    indices.clear();

    for (const auto &shape : shapes) {
        for (const auto &index : shape.mesh.indices) {
            Vertex vertex{};
            if (index.vertex_index >= 0) {
                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],  // X
                    attrib.vertices[3 * index.vertex_index + 1],  // Y
                    attrib.vertices[3 * index.vertex_index + 2],  // Z
                };

                auto colorIndex = 3 * index.vertex_index + 2;
                if (colorIndex < attrib.colors.size()) {
                    vertex.color = {
                        attrib.colors[colorIndex - 2],  // R
                        attrib.colors[colorIndex - 1],  // G
                        attrib.colors[colorIndex - 0],  // B
                    };
                } else {
                    vertex.color = {1.f, 1.f, 1.f};  // default color
                }
            }

            if (index.normal_index >= 0) {
                vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],  // X
                    attrib.normals[3 * index.normal_index + 1],  // Y
                    attrib.normals[3 * index.normal_index + 2],  // Z
                };
            }

            if (index.texcoord_index >= 0) {
                vertex.uv = {
                    attrib.texcoords[2 * index.texcoord_index + 0],  // X
                    attrib.texcoords[2 * index.texcoord_index + 1],  // Y
                };
            }

            vertices.push_back(vertex);
        }
    }
}

}  // namespace pve