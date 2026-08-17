#include "systems/ui_render_system.hpp"

#include <cassert>
#include <cstring>
#include <stdexcept>

namespace pve {

std::vector<VkVertexInputBindingDescription> UIVertex::getBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(UIVertex);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> UIVertex::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

    attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(UIVertex, position)});
    attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(UIVertex, color)});

    return attributeDescriptions;
}

UIRenderSystem::UIRenderSystem(PveDevice &device, VkRenderPass renderPass) 
    : pveDevice{device}, vertexBuffer{VK_NULL_HANDLE}, vertexBufferMemory{VK_NULL_HANDLE},
      indexBuffer{VK_NULL_HANDLE}, indexBufferMemory{VK_NULL_HANDLE},
      vertexBufferSize{0}, indexBufferSize{0} {
    createPipelineLayout();
    createPipeline(renderPass);
}

UIRenderSystem::~UIRenderSystem() {
    if (vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(pveDevice.device(), vertexBuffer, nullptr);
        vkFreeMemory(pveDevice.device(), vertexBufferMemory, nullptr);
    }
    if (indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(pveDevice.device(), indexBuffer, nullptr);
        vkFreeMemory(pveDevice.device(), indexBufferMemory, nullptr);
    }
    
    vkDestroyPipelineLayout(pveDevice.device(), pipelineLayout, nullptr);
}

void UIRenderSystem::createPipelineLayout() {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(pveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void UIRenderSystem::createPipeline(VkRenderPass renderPass) {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    PvePipeline::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    pipelineConfig.bindingDescriptions = UIVertex::getBindingDescriptions();
    pipelineConfig.attributeDescriptions = UIVertex::getAttributeDescriptions();

    pvePipeline = std::make_unique<PvePipeline>(
        pveDevice,
        "shaders/compiled/ui_shader.vert.spv",
        "shaders/compiled/ui_shader.frag.spv",
        pipelineConfig);
}

void UIRenderSystem::renderGameObjects(FrameInfo &frameInfo, const std::vector<UIButton> &buttons, int mouseX, int mouseY) {
    pvePipeline->bind(frameInfo.commandBuffer);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 0,
                           nullptr, 0, nullptr);

    // Calculate total size needed
    size_t totalVertices = buttons.size() * 4; // 4 vertices per button
    size_t totalIndices = buttons.size() * 6; // 6 indices per button
    
    VkDeviceSize newVertexBufferSize = sizeof(UIVertex) * totalVertices;
    VkDeviceSize newIndexBufferSize = sizeof(uint32_t) * totalIndices;

    // Reallocate buffers if needed
    if (vertexBuffer == VK_NULL_HANDLE || vertexBufferSize < newVertexBufferSize) {
        if (vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(pveDevice.device(), vertexBuffer, nullptr);
            vkFreeMemory(pveDevice.device(), vertexBufferMemory, nullptr);
        }
        pveDevice.createBuffer(newVertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              vertexBuffer, vertexBufferMemory);
        vertexBufferSize = newVertexBufferSize;
    }
    
    if (indexBuffer == VK_NULL_HANDLE || indexBufferSize < newIndexBufferSize) {
        if (indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(pveDevice.device(), indexBuffer, nullptr);
            vkFreeMemory(pveDevice.device(), indexBufferMemory, nullptr);
        }
        pveDevice.createBuffer(newIndexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              indexBuffer, indexBufferMemory);
        indexBufferSize = newIndexBufferSize;
    }

    // Fill vertex buffer
    std::vector<UIVertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t vertexOffset = 0;
    
    for (const auto &button : buttons) {
        // Check if mouse is hovering over button
        bool isHovering = mouseX >= button.position.x && mouseX <= button.position.x + button.size.x &&
                          mouseY >= button.position.y && mouseY <= button.position.y + button.size.y;

        glm::vec4 color = isHovering ? button.hoverColor : button.color;

        // Create quad vertices for button
        vertices.push_back({{button.position.x, button.position.y}, color});
        vertices.push_back({{button.position.x + button.size.x, button.position.y}, color});
        vertices.push_back({{button.position.x + button.size.x, button.position.y + button.size.y}, color});
        vertices.push_back({{button.position.x, button.position.y + button.size.y}, color});

        indices.push_back(vertexOffset + 0);
        indices.push_back(vertexOffset + 1);
        indices.push_back(vertexOffset + 2);
        indices.push_back(vertexOffset + 0);
        indices.push_back(vertexOffset + 2);
        indices.push_back(vertexOffset + 3);
        
        vertexOffset += 4;
    }

    // Upload vertex data
    void *data;
    vkMapMemory(pveDevice.device(), vertexBufferMemory, 0, vertices.size() * sizeof(UIVertex), 0, &data);
    memcpy(data, vertices.data(), vertices.size() * sizeof(UIVertex));
    vkUnmapMemory(pveDevice.device(), vertexBufferMemory);

    // Upload index data
    vkMapMemory(pveDevice.device(), indexBufferMemory, 0, indices.size() * sizeof(uint32_t), 0, &data);
    memcpy(data, indices.data(), indices.size() * sizeof(uint32_t));
    vkUnmapMemory(pveDevice.device(), indexBufferMemory);

    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(frameInfo.commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(frameInfo.commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                      sizeof(glm::mat4), &modelMatrix);

    vkCmdDrawIndexed(frameInfo.commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

void UIRenderSystem::renderText(FrameInfo &frameInfo, const std::string &text, glm::vec2 position, glm::vec4 color) {
    // Simple text rendering using colored quads (placeholder for real font rendering)
    // In a full implementation, this would use a texture atlas with font glyphs
    
    pvePipeline->bind(frameInfo.commandBuffer);
    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 0,
                           nullptr, 0, nullptr);

    float charWidth = 10.0f;
    float charHeight = 20.0f;
    float spacing = 2.0f;

    // Calculate total size needed
    size_t numChars = 0;
    for (char c : text) {
        if (c != ' ') numChars++;
    }
    size_t totalVertices = numChars * 4;
    size_t totalIndices = numChars * 6;
    
    VkDeviceSize newVertexBufferSize = sizeof(UIVertex) * totalVertices;
    VkDeviceSize newIndexBufferSize = sizeof(uint32_t) * totalIndices;

    // Reallocate buffers if needed
    if (vertexBuffer == VK_NULL_HANDLE || vertexBufferSize < newVertexBufferSize) {
        if (vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(pveDevice.device(), vertexBuffer, nullptr);
            vkFreeMemory(pveDevice.device(), vertexBufferMemory, nullptr);
        }
        pveDevice.createBuffer(newVertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              vertexBuffer, vertexBufferMemory);
        vertexBufferSize = newVertexBufferSize;
    }
    
    if (indexBuffer == VK_NULL_HANDLE || indexBufferSize < newIndexBufferSize) {
        if (indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(pveDevice.device(), indexBuffer, nullptr);
            vkFreeMemory(pveDevice.device(), indexBufferMemory, nullptr);
        }
        pveDevice.createBuffer(newIndexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              indexBuffer, indexBufferMemory);
        indexBufferSize = newIndexBufferSize;
    }

    // Fill vertex buffer
    std::vector<UIVertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t vertexOffset = 0;
    float currentX = position.x;
    
    for (char c : text) {
        if (c == ' ') {
            currentX += charWidth + spacing;
            continue;
        }

        // Simple character representation as a small quad
        vertices.push_back({{currentX, position.y}, color});
        vertices.push_back({{currentX + charWidth, position.y}, color});
        vertices.push_back({{currentX + charWidth, position.y + charHeight}, color});
        vertices.push_back({{currentX, position.y + charHeight}, color});

        indices.push_back(vertexOffset + 0);
        indices.push_back(vertexOffset + 1);
        indices.push_back(vertexOffset + 2);
        indices.push_back(vertexOffset + 0);
        indices.push_back(vertexOffset + 2);
        indices.push_back(vertexOffset + 3);
        
        vertexOffset += 4;
        currentX += charWidth + spacing;
    }

    // Upload vertex data
    void *data;
    vkMapMemory(pveDevice.device(), vertexBufferMemory, 0, vertices.size() * sizeof(UIVertex), 0, &data);
    memcpy(data, vertices.data(), vertices.size() * sizeof(UIVertex));
    vkUnmapMemory(pveDevice.device(), vertexBufferMemory);

    // Upload index data
    vkMapMemory(pveDevice.device(), indexBufferMemory, 0, indices.size() * sizeof(uint32_t), 0, &data);
    memcpy(data, indices.data(), indices.size() * sizeof(uint32_t));
    vkUnmapMemory(pveDevice.device(), indexBufferMemory);

    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(frameInfo.commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(frameInfo.commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                      sizeof(glm::mat4), &modelMatrix);

    vkCmdDrawIndexed(frameInfo.commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

}  // namespace pve
