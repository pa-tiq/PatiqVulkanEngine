#include "pve/pve_renderer.hpp"

#include <array>
#include <cassert>
#include <stdexcept>

namespace pve {

PveRenderer::PveRenderer(PveWindow &window, PveDevice &device)
    : pveWindow{window}, pveDevice{device} {
    recreateSwapChain();
    createCommandBuffers();
}

PveRenderer::~PveRenderer() { freeCommandBuffers(); }

void PveRenderer::recreateSwapChain() {
    auto extent = pveWindow.getExtent();
    while (extent.width == 0 || extent.height == 0) {
        extent = pveWindow.getExtent();
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(pveDevice.device());
    if (pveSwapChain == nullptr) {
        pveSwapChain = std::make_unique<PveSwapChain>(pveDevice, extent);
    } else {
        std::shared_ptr<PveSwapChain> oldSwapChain = std::move(pveSwapChain);
        // the move function allows us to create a copy, but setting pveSwapChain to a nullptr
        pveSwapChain = std::make_unique<PveSwapChain>(pveDevice, extent, oldSwapChain);
        if (!oldSwapChain->compareSwapFormats(*pveSwapChain.get())) {
            throw std::runtime_error("Swap chain image(or depth) format has changed");
        }
    }
}

void PveRenderer::createCommandBuffers() {
    commandBuffers.resize(PveSwapChain::MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = pveDevice.getCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(pveDevice.device(), &allocInfo, commandBuffers.data()) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers");
    }
}

void PveRenderer::freeCommandBuffers() {
    vkFreeCommandBuffers(pveDevice.device(), pveDevice.getCommandPool(),
                         static_cast<uint32_t>(commandBuffers.size()),
                         commandBuffers.data());
    commandBuffers.clear();
}

VkCommandBuffer PveRenderer::beginFrame() {
    assert(!isFrameStarted && "Can't call beginFrame while already in progress");

    auto result = pveSwapChain->acquireNextImage(&currentImageIndex);

    if (result ==
        VK_ERROR_OUT_OF_DATE_KHR) {  // This error can occur after the window has been resized
        recreateSwapChain();
        return nullptr;  // indicates the frame has not successfully started
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image");
    }

    isFrameStarted = true;

    auto commandBuffer = getCurrentCommandBuffer();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer");
    }
    return commandBuffer;
}

void PveRenderer::endFrame() {
    assert(isFrameStarted && "Can't call endFrame while frame not in progress");
    auto commandBuffer = getCurrentCommandBuffer();
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer");
    }
    auto result = pveSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);

    // VK_SUBOPTIMAL_KHR:  Swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        pveWindow.wasWindowResized()) {
        pveWindow.resetWindowResizedFlag();
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image");
    }
    isFrameStarted = false;
    currentFrameIndex = (currentFrameIndex + 1) % PveSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void PveRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
    assert(isFrameStarted &&
           "Can't call beginSwapChainRenderPass while frame not in progress");
    assert(commandBuffer == getCurrentCommandBuffer() &&
           "Can't begin render pass on command buffer from a different frame");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = pveSwapChain->getRenderPass();
    renderPassInfo.framebuffer = pveSwapChain->getFrameBuffer(currentImageIndex);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = pveSwapChain->getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

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
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void PveRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
    assert(isFrameStarted &&
           "Can't call endSwapChaniRenderPass while frame not in progress");
    assert(commandBuffer == getCurrentCommandBuffer() &&
           "Can't end render pass on command buffer from a different frame");

    vkCmdEndRenderPass(commandBuffer);
}

}  // namespace pve
