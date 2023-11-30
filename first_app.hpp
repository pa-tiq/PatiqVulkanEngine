#pragma once

#include "pve_window.hpp"
#include "pve_pipeline.hpp"
#include "pve_device.hpp"
#include "pve_swap_chain.hpp"

#include <memory>
#include <vector>

namespace pve
{
    class FirstApp
    {
    public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;

        FirstApp();
        ~FirstApp();

        FirstApp(const FirstApp &) = delete;
        FirstApp &operator=(const FirstApp &) = delete;

        void run();

    private:
        void createPipelineLayout();
        void createPipeline();
        void createCommandBuffers();
        void drawFrame();

        PveWindow pveWindow{WIDTH, HEIGHT, "Hello Vulkan!"};
        PveDevice pveDevice{pveWindow};
        PveSwapChain pveSwapChain{pveDevice, pveWindow.getExtent()};
        // a smart pointer simulates a pointer but with the addition of automatic memory management
        std::unique_ptr<PvePipeline> pvePipeline;
        VkPipelineLayout pipelineLayout;
        std::vector<VkCommandBuffer> commandBuffer;
    };
}
