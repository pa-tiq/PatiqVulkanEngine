#include "first_app.hpp"

#define GLM_FORCE_RADIANS            // No matter what system i'm in, angles are in radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE  // Forces GLM to expect depth buffer values to range from 0 to 1 instead of -1 to 1 (the opengl standard)
#include <array>
#include <glm/glm.hpp>
#include <stdexcept>

namespace pve {

// temporary
struct SimplePushConstantData {
    glm::vec2 offset;
    alignas(16) glm::vec3 color; // alignas: tutorial 9 at 09:00
};

static glm::vec3 red = {1.0f, 0.0f, 0.0f};
static glm::vec3 green = {0.0f, 1.0f, 0.0f};
static glm::vec3 blue = {0.0f, 0.0f, 1.0f};

FirstApp::FirstApp() {
    loadModels();
    createPipelineLayout();
    recreateSwapChain();
    createCommandBuffers();
}

FirstApp::~FirstApp() {
    vkDestroyPipelineLayout(pveDevice.device(), pipelineLayout, nullptr);
}

void FirstApp::run() {
    // while the window doesn't want to close, poll window events
    while (!pveWindow.shouldClose()) {
        glfwPollEvents();
        drawFrame();
    }

    // this makes the CPU block until all GPU operations have completed
    vkDeviceWaitIdle(pveDevice.device());
}

void FirstApp::sierpinski(
    std::vector<PveModel::Vertex> &vertices,
    int depth,
    glm::vec2 left,
    glm::vec2 right,
    glm::vec2 top) {
    if (depth <= 0) {
        vertices.push_back({top, red});
        vertices.push_back({right, green});
        vertices.push_back({left, blue});
    } else {
        auto leftTop = 0.5f * (left + top);
        auto rightTop = 0.5f * (right + top);
        auto leftRight = 0.5f * (left + right);
        sierpinski(vertices, depth - 1, left, leftRight, leftTop);
        sierpinski(vertices, depth - 1, leftRight, right, rightTop);
        sierpinski(vertices, depth - 1, leftTop, rightTop, top);
    }
}

void FirstApp::loadModels() {
    // this will initialize vertex data positions
    std::vector<PveModel::Vertex> vertices{
        {{0.0f, -0.5f}, red},
        {{0.5f, 0.5f}, green},
        {{-0.5f, 0.5f}, blue}};
    // std::vector<PveModel::Vertex> vertices{};
    // sierpinski(vertices, 5,
    //            {-0.5f, 0.5f},
    //            {0.5f, 0.5f},
    //            {0.0f, -0.5f});
    pveModel = std::make_unique<PveModel>(pveDevice, vertices);
}

void FirstApp::createPipelineLayout() {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SimplePushConstantData);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(pveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }
}

void FirstApp::createPipeline() {
    assert(pveSwapChain != nullptr && "Cannot create pipeline before swap chain");
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    PvePipeline::defaultPipelineConfigInfo(pipelineConfig);
    // a render pass describes the structure and format of our frame buffer objects and their attachments
    // it's a blueprint that tells a graphics pipeline object what layout to expect from the output frame buffer
    // when it's time to actually render, the graphics pipeline is already prepared to output to our frame buffer,
    // as long as the passed frame buffer object is setup in a way that is compatible with what we specified in the render pass.
    // multiple subpasses can be grouped together into a single render pass
    pipelineConfig.renderPass = pveSwapChain->getRenderPass();
    pipelineConfig.pipelineLayout = pipelineLayout;
    pvePipeline = std::make_unique<PvePipeline>(
        pveDevice,
        "shaders/simple_shader.vert.spv",
        "shaders/simple_shader.frag.spv",
        pipelineConfig);
}

void FirstApp::recreateSwapChain() {
    auto extent = pveWindow.getExtent();
    while (extent.width == 0 || extent.height == 0) {
        extent = pveWindow.getExtent();
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(pveDevice.device());
    if (pveSwapChain == nullptr) {
        pveSwapChain = std::make_unique<PveSwapChain>(pveDevice, extent);
    } else {
        // the move function allows us to create a copy, but setting pveSwapChain to a nullptr
        pveSwapChain = std::make_unique<PveSwapChain>(pveDevice, extent, std::move(pveSwapChain));
        if (pveSwapChain->imageCount() != commandBuffers.size()) {
            freeCommandBuffers();
            createCommandBuffers();
        }
    }

    // if render pass is compatible there's no need to recreate the pipeline
    createPipeline();
}

void FirstApp::createCommandBuffers() {
    commandBuffers.resize(pveSwapChain->imageCount());
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = pveDevice.getCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(pveDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers");
    }
}

void FirstApp::freeCommandBuffers() {
    vkFreeCommandBuffers(pveDevice.device(), pveDevice.getCommandPool(), static_cast<float>(commandBuffers.size()), commandBuffers.data());
    commandBuffers.clear();
}

void FirstApp::recordCommandBuffer(int imageIndex) {
    // Animation
    static int frame = 0;
    frame = (frame + 1) % 1000;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = pveSwapChain->getRenderPass();
    renderPassInfo.framebuffer = pveSwapChain->getFrameBuffer(imageIndex);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = pveSwapChain->getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // every frame we record a command buffer and dynamically set the viewport just before submittig the buffer to be executed
    // this way, we'll always be using the correct window size even if the swap chain changes
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(pveSwapChain->getSwapChainExtent().width);
    viewport.height = static_cast<float>(pveSwapChain->getSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{{0, 0}, pveSwapChain->getSwapChainExtent()};
    vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);
    vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

    // commandBuffer in each render pass will bind our graphics pipeline,
    // then bind our model, which contains the vertex data, and then record a
    // command buffer to draw every vertex contained by the model
    pvePipeline->bind(commandBuffers[imageIndex]);
    pveModel->bind(commandBuffers[imageIndex]);

    for (int j = 0; j < 4; j++) {
        SimplePushConstantData push{};
        push.offset = {-0.5f + frame * 0.002f, -0.4f + j * 0.25f};
        push.color = {0.0f, 0.0f, 0.2f + 0.2f * j};
        vkCmdPushConstants(
            commandBuffers[imageIndex],
            pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(SimplePushConstantData),
            &push);

        pveModel->draw(commandBuffers[imageIndex]);
    }

    vkCmdEndRenderPass(commandBuffers[imageIndex]);

    if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer");
    }
}

void FirstApp::drawFrame() {
    uint32_t imageIndex;
    auto result = pveSwapChain->acquireNextImage(&imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {  // This error can occur after the window has been resized
        recreateSwapChain();
        return;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image");
    }
    recordCommandBuffer(imageIndex);
    result = pveSwapChain->submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);

    // VK_SUBOPTIMAL_KHR:  Swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || pveWindow.wasWindowResized()) {
        pveWindow.resetWindowResizedFlag();
        recreateSwapChain();
        return;
    }

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image");
    }
}

}  // namespace pve
