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

    // Disable depth testing for UI - UI should always render on top
    pipelineConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
    pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;

    pvePipeline = std::make_unique<PvePipeline>(
        pveDevice,
        "shaders/compiled/ui_shader.vert.spv",
        "shaders/compiled/ui_shader.frag.spv",
        pipelineConfig);
}

// Helper: builds an orthographic projection matrix that maps pixel coordinates to NDC.
// Maps (0,0)-(screenWidth,screenHeight) to (-1,-1)-(1,1) in Vulkan clip space.
static glm::mat4 makeOrthoProjection(float screenWidth, float screenHeight) {
    glm::mat4 proj = glm::mat4(1.0f);
    proj[0][0] =  2.0f / screenWidth;
    proj[1][1] =  2.0f / screenHeight;
    proj[3][0] = -1.0f;
    proj[3][1] = -1.0f;
    return proj;
}

void UIRenderSystem::ensureBufferCapacity(VkDeviceSize requiredVertexSize, VkDeviceSize requiredIndexSize) {
    if (vertexBuffer == VK_NULL_HANDLE || vertexBufferSize < requiredVertexSize) {
        if (vertexBuffer != VK_NULL_HANDLE) {
            // Only safe to destroy here if not currently bound in a command buffer.
            // We grow buffers once and reuse, so this should only happen on the first frame
            // or when the UI grows significantly.
            vkDeviceWaitIdle(pveDevice.device());
            vkDestroyBuffer(pveDevice.device(), vertexBuffer, nullptr);
            vkFreeMemory(pveDevice.device(), vertexBufferMemory, nullptr);
        }
        // Allocate with some extra room to avoid frequent reallocations
        VkDeviceSize allocSize = requiredVertexSize * 2;
        pveDevice.createBuffer(allocSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              vertexBuffer, vertexBufferMemory);
        vertexBufferSize = allocSize;
    }

    if (indexBuffer == VK_NULL_HANDLE || indexBufferSize < requiredIndexSize) {
        if (indexBuffer != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(pveDevice.device());
            vkDestroyBuffer(pveDevice.device(), indexBuffer, nullptr);
            vkFreeMemory(pveDevice.device(), indexBufferMemory, nullptr);
        }
        VkDeviceSize allocSize = requiredIndexSize * 2;
        pveDevice.createBuffer(allocSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              indexBuffer, indexBufferMemory);
        indexBufferSize = allocSize;
    }
}

void UIRenderSystem::renderGameObjects(FrameInfo &frameInfo, const std::vector<UIButton> &buttons, int mouseX, int mouseY) {
    if (buttons.empty()) return;

    pvePipeline->bind(frameInfo.commandBuffer);

    // Calculate total size needed (buttons + text labels)
    size_t totalVertices = buttons.size() * 4; // 4 vertices per button quad
    size_t totalIndices = buttons.size() * 6;  // 6 indices per button quad

    // Pre-count text character quads for all button labels
    for (const auto &button : buttons) {
        for (char c : button.text) {
            if (c != ' ') {
                totalVertices += 4;
                totalIndices += 6;
            }
        }
    }

    VkDeviceSize newVertexBufferSize = sizeof(UIVertex) * totalVertices;
    VkDeviceSize newIndexBufferSize = sizeof(uint32_t) * totalIndices;

    ensureBufferCapacity(newVertexBufferSize, newIndexBufferSize);

    // Fill vertex and index data for buttons AND their text labels in one batch
    std::vector<UIVertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t vertexOffset = 0;
    
    float charWidth = 10.0f;
    float charHeight = 16.0f;
    float spacing = 2.0f;

    for (const auto &button : buttons) {
        // Check if mouse is hovering over button
        bool isHovering = mouseX >= button.position.x && mouseX <= button.position.x + button.size.x &&
                          mouseY >= button.position.y && mouseY <= button.position.y + button.size.y;

        glm::vec4 color = isHovering ? button.hoverColor : button.color;

        // Create quad vertices for button background
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

        // Render text label centered on the button
        float textWidth = 0.0f;
        for (char c : button.text) {
            textWidth += charWidth + spacing;
        }
        textWidth -= spacing; // remove trailing spacing

        float textX = button.position.x + (button.size.x - textWidth) / 2.0f;
        float textY = button.position.y + (button.size.y - charHeight) / 2.0f;

        // Hacker green text
        glm::vec4 textColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

        for (char c : button.text) {
            if (c == ' ') {
                textX += charWidth + spacing;
                continue;
            }

            vertices.push_back({{textX, textY}, textColor});
            vertices.push_back({{textX + charWidth, textY}, textColor});
            vertices.push_back({{textX + charWidth, textY + charHeight}, textColor});
            vertices.push_back({{textX, textY + charHeight}, textColor});

            indices.push_back(vertexOffset + 0);
            indices.push_back(vertexOffset + 1);
            indices.push_back(vertexOffset + 2);
            indices.push_back(vertexOffset + 0);
            indices.push_back(vertexOffset + 2);
            indices.push_back(vertexOffset + 3);
            
            vertexOffset += 4;
            textX += charWidth + spacing;
        }
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

    // Use orthographic projection to map pixel coordinates to NDC
    glm::mat4 orthoProjection = makeOrthoProjection(frameInfo.screenWidth, frameInfo.screenHeight);
    vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                      sizeof(glm::mat4), &orthoProjection);

    vkCmdDrawIndexed(frameInfo.commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

void UIRenderSystem::renderText(FrameInfo &frameInfo, const std::string &text, glm::vec2 position, glm::vec4 color) {
    // Simple text rendering using colored quads (placeholder for real font rendering)
    // In a full implementation, this would use a texture atlas with font glyphs
    
    // Count non-space characters
    size_t numChars = 0;
    for (char c : text) {
        if (c != ' ') numChars++;
    }
    if (numChars == 0) return;

    pvePipeline->bind(frameInfo.commandBuffer);

    float charWidth = 10.0f;
    float charHeight = 20.0f;
    float spacing = 2.0f;

    size_t totalVertices = numChars * 4;
    size_t totalIndices = numChars * 6;
    
    VkDeviceSize newVertexBufferSize = sizeof(UIVertex) * totalVertices;
    VkDeviceSize newIndexBufferSize = sizeof(uint32_t) * totalIndices;

    ensureBufferCapacity(newVertexBufferSize, newIndexBufferSize);

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

    // Use orthographic projection to map pixel coordinates to NDC
    glm::mat4 orthoProjection = makeOrthoProjection(frameInfo.screenWidth, frameInfo.screenHeight);
    vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                      sizeof(glm::mat4), &orthoProjection);

    vkCmdDrawIndexed(frameInfo.commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

}  // namespace pve
