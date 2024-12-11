#pragma once

#include <cassert>
#include <memory>
#include <vector>

#include "pve_device.hpp"
#include "pve_swap_chain.hpp"
#include "pve_window.hpp"

namespace pve {
class PveRenderer {
   public:
    PveRenderer(PveWindow &window, PveDevice &device);
    ~PveRenderer();

    PveRenderer(const PveRenderer &) = delete;
    PveRenderer &operator=(const PveRenderer &) = delete;

    VkRenderPass getSwapChainRenderPass() const { return pveSwapChain->getRenderPass(); }
    float getAspectRatio() const { return pveSwapChain->extentAspectRatio(); }
    bool isFrameInProgress() const { return isFrameStarted; }

    VkCommandBuffer getCurrentCommandBuffer() const {
        assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
        return commandBuffers[currentFrameIndex];
    }

    int getFrameIndex() const {
        assert(isFrameStarted && "Cannot get frame index when frame not in progress");
        return currentFrameIndex;
    }

    VkCommandBuffer beginFrame();
    void endFrame();
    void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
    void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

   private:
    // Renderer: swapchain, command buffers and draw frame
    void createCommandBuffers();
    void freeCommandBuffers();
    void drawFrame();
    void recreateSwapChain();

    PveWindow &pveWindow;
    PveDevice &pveDevice;
    std::unique_ptr<PveSwapChain> pveSwapChain;
    std::vector<VkCommandBuffer> commandBuffers;

    uint32_t currentImageIndex;
    int currentFrameIndex{0};
    bool isFrameStarted{false};
};
}  // namespace pve
