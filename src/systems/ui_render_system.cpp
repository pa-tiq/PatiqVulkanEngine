#include "systems/ui_render_system.hpp"

#include <cassert>
#include <cstring>
#include <cctype>
#include <stdexcept>

namespace pve {

// Simple 3x5 font (3 bits wide, 5 rows) for A-Z
static const uint8_t font3x5[26][5] = {
    {0x2, 0x5, 0x7, 0x5, 0x5}, // A
    {0x6, 0x5, 0x6, 0x5, 0x6}, // B
    {0x3, 0x4, 0x4, 0x4, 0x3}, // C
    {0x6, 0x5, 0x5, 0x5, 0x6}, // D
    {0x7, 0x4, 0x6, 0x4, 0x7}, // E
    {0x7, 0x4, 0x6, 0x4, 0x4}, // F
    {0x3, 0x4, 0x5, 0x5, 0x3}, // G
    {0x5, 0x5, 0x7, 0x5, 0x5}, // H
    {0x7, 0x2, 0x2, 0x2, 0x7}, // I
    {0x1, 0x1, 0x1, 0x5, 0x2}, // J
    {0x5, 0x5, 0x6, 0x5, 0x5}, // K
    {0x4, 0x4, 0x4, 0x4, 0x7}, // L
    {0x5, 0x7, 0x5, 0x5, 0x5}, // M
    {0x5, 0x5, 0x7, 0x5, 0x5}, // N
    {0x2, 0x5, 0x5, 0x5, 0x2}, // O
    {0x6, 0x5, 0x6, 0x4, 0x4}, // P
    {0x2, 0x5, 0x5, 0x6, 0x3}, // Q
    {0x6, 0x5, 0x6, 0x5, 0x5}, // R
    {0x3, 0x4, 0x2, 0x1, 0x6}, // S
    {0x7, 0x2, 0x2, 0x2, 0x2}, // T
    {0x5, 0x5, 0x5, 0x5, 0x2}, // U
    {0x5, 0x5, 0x5, 0x2, 0x2}, // V
    {0x5, 0x5, 0x5, 0x7, 0x5}, // W
    {0x5, 0x5, 0x2, 0x5, 0x5}, // X
    {0x5, 0x5, 0x2, 0x2, 0x2}, // Y
    {0x7, 0x1, 0x2, 0x4, 0x7}, // Z
};

static void pushCharQuads(char c, float startX, float startY, float scale, glm::vec4 color, 
                          std::vector<UIVertex>& vertices, std::vector<uint32_t>& indices, uint32_t& vertexOffset) {
    c = std::toupper(c);
    if (c < 'A' || c > 'Z') return;
    int idx = c - 'A';
    for (int row = 0; row < 5; ++row) {
        uint8_t bits = font3x5[idx][row];
        for (int col = 0; col < 3; ++col) {
            if (bits & (1 << (2 - col))) {
                float px = startX + col * scale;
                float py = startY + row * scale;
                vertices.push_back({{px, py}, color});
                vertices.push_back({{px + scale, py}, color});
                vertices.push_back({{px + scale, py + scale}, color});
                vertices.push_back({{px, py + scale}, color});
                indices.push_back(vertexOffset + 0);
                indices.push_back(vertexOffset + 1);
                indices.push_back(vertexOffset + 2);
                indices.push_back(vertexOffset + 0);
                indices.push_back(vertexOffset + 2);
                indices.push_back(vertexOffset + 3);
                vertexOffset += 4;
            }
        }
    }
}

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

void UIRenderSystem::renderGameObjects(const std::vector<UIButton> &buttons, int mouseX, int mouseY) {
    if (buttons.empty()) return;

    // We no longer bind the pipeline here, it will be done in finishRender.
    // Ensure lists are ready (cleared at end of previous frame)

    // Calculate total size needed (buttons + text labels)
    size_t totalVertices = buttons.size() * 4; // 4 vertices per button quad
    size_t totalIndices = buttons.size() * 6;  // 6 indices per button quad

    // Pre-count text character quads for all button labels (15 pixels max per char)
    for (const auto &button : buttons) {
        for (char c : button.text) {
            if (c != ' ') {
                totalVertices += 15 * 4;
                totalIndices += 15 * 6;
            }
        }
    }

    VkDeviceSize newVertexBufferSize = sizeof(UIVertex) * totalVertices;
    VkDeviceSize newIndexBufferSize = sizeof(uint32_t) * totalIndices;

    ensureBufferCapacity(newVertexBufferSize, newIndexBufferSize);

    // Fill vertex and index data for buttons AND their text labels in one batch
    uint32_t vertexOffset = static_cast<uint32_t>(uiVertices.size());
    
    float pixelScale = 3.0f; // Scale for font
    float charWidth = 3.0f * pixelScale;
    float charHeight = 5.0f * pixelScale;
    float spacing = 1.0f * pixelScale;

    for (const auto &button : buttons) {
        // Check if mouse is hovering over button
        bool isHovering = mouseX >= button.position.x && mouseX <= button.position.x + button.size.x &&
                          mouseY >= button.position.y && mouseY <= button.position.y + button.size.y;

        glm::vec4 color = isHovering ? button.hoverColor : button.color;
        
        float bx = button.position.x;
        float by = button.position.y;
        float bw = button.size.x;
        float bh = button.size.y;
        
        // Scale button slightly on hover
        if (isHovering) {
            float scaleAmt = 0.05f; // 5% scale
            float dw = bw * scaleAmt;
            float dh = bh * scaleAmt;
            bx -= dw / 2.0f;
            by -= dh / 2.0f;
            bw += dw;
            bh += dh;
        }

        // Create quad vertices for button background
        uiVertices.push_back({{bx, by}, color});
        uiVertices.push_back({{bx + bw, by}, color});
        uiVertices.push_back({{bx + bw, by + bh}, color});
        uiVertices.push_back({{bx, by + bh}, color});

        uiIndices.push_back(vertexOffset + 0);
        uiIndices.push_back(vertexOffset + 1);
        uiIndices.push_back(vertexOffset + 2);
        uiIndices.push_back(vertexOffset + 0);
        uiIndices.push_back(vertexOffset + 2);
        uiIndices.push_back(vertexOffset + 3);
        
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
            pushCharQuads(c, textX, textY, pixelScale, textColor, uiVertices, uiIndices, vertexOffset);
            textX += charWidth + spacing;
        }
    }
}

void UIRenderSystem::renderText(const std::string &text, glm::vec2 position, glm::vec4 color) {
    // Count non-space characters
    size_t numChars = 0;
    for (char c : text) {
        if (c != ' ') numChars++;
    }

    if (numChars == 0) return;

    float pixelScale = 4.0f; // Scale for font
    float charWidth = 3.0f * pixelScale;
    float charHeight = 5.0f * pixelScale;
    float spacing = 1.0f * pixelScale;

    // Append to member buffers
    uint32_t vertexOffset = static_cast<uint32_t>(uiVertices.size());
    float currentX = position.x;
    
    for (char c : text) {
        if (c == ' ') {
            currentX += charWidth + spacing;
            continue;
        }
        
        pushCharQuads(c, currentX, position.y, pixelScale, color, uiVertices, uiIndices, vertexOffset);
        currentX += charWidth + spacing;
    }
}

void UIRenderSystem::finishRender(FrameInfo &frameInfo) {
    if (uiVertices.empty() || uiIndices.empty()) return;

    VkDeviceSize newVertexBufferSize = sizeof(UIVertex) * uiVertices.size();
    VkDeviceSize newIndexBufferSize = sizeof(uint32_t) * uiIndices.size();

    ensureBufferCapacity(newVertexBufferSize, newIndexBufferSize);

    // Upload vertex data
    void *data;
    vkMapMemory(pveDevice.device(), vertexBufferMemory, 0, newVertexBufferSize, 0, &data);
    memcpy(data, uiVertices.data(), newVertexBufferSize);
    vkUnmapMemory(pveDevice.device(), vertexBufferMemory);

    // Upload index data
    vkMapMemory(pveDevice.device(), indexBufferMemory, 0, newIndexBufferSize, 0, &data);
    memcpy(data, uiIndices.data(), newIndexBufferSize);
    vkUnmapMemory(pveDevice.device(), indexBufferMemory);

    pvePipeline->bind(frameInfo.commandBuffer);

    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(frameInfo.commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(frameInfo.commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // Use orthographic projection to map pixel coordinates to NDC
    glm::mat4 orthoProjection = makeOrthoProjection(frameInfo.screenWidth, frameInfo.screenHeight);
    vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                      sizeof(glm::mat4), &orthoProjection);

    vkCmdDrawIndexed(frameInfo.commandBuffer, static_cast<uint32_t>(uiIndices.size()), 1, 0, 0, 0);

    // Clear vectors for next frame
    uiVertices.clear();
    uiIndices.clear();
}

}  // namespace pve
