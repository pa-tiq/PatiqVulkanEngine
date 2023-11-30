#include "first_app.hpp"

#include <stdexcept>

namespace pve
{

    FirstApp::FirstApp(){
        createPipelineLayout();
        createPipeline();
        createCommandBuffers();
    }

    FirstApp::~FirstApp(){
        vkDestroyPipelineLayout(pveDevice.device(), pipelineLayout, nullptr);
    }

    void FirstApp::run(){
        // while the window doesn't want to close, poll window events
        while(!pveWindow.shouldClose()){
            glfwPollEvents();
        }
    }

    void FirstApp::createPipelineLayout(){
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        if(vkCreatePipelineLayout(pveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS){
            throw std::runtime_error("Failed to create pipeline layout");
        }
    }

    void FirstApp::createPipeline(){
        auto pipelineConfig = PvePipeline::defaultPipelineConfigInfo(pveSwapChain.width(), pveSwapChain.height());
        // a render pass describes the structure and format of our frame buffer objects and their attachments
        // it's a blueprint that tells a graphics pipeline object what layout to expect from the output frame buffer
        // when it's time to actually render, the graphics pipeline is already prepared to output to our frame buffer,
        // as long as the passed frame buffer object is setup in a way that is compatible with what we specified in the render pass.
        // multiple subpasses can be grouped together into a single render pass
        pipelineConfig.renderPass = pveSwapChain.getRenderPass();
        pipelineConfig.pipelineLayout = pipelineLayout;
        pvePipeline = std::make_unique<PvePipeline>(
            pveDevice,
            "shaders/simple_shader.vert.spv",
            "shaders/simple_shader.frag.spv",
            pipelineConfig
        );
    }

    void FirstApp::createCommandBuffers(){
        // commandBuffers.resize(pveSwapChain.imageCount());
        // VkCommandBufferAllocateInfo allocInfo{};
        // allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        // allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        // allocInfo.commandPool = pveDevice.getCommandPool();
        // allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    }
    void FirstApp::drawFrame(){}

} // namespace pve
