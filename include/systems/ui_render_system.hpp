#pragma once

#include <memory>
#include <vector>

#include "pve/pve_device.hpp"
#include "pve/pve_frame_info.hpp"
#include "pve/pve_pipeline.hpp"

namespace pve {

struct UIVertex {
    glm::vec2 position;
    glm::vec4 color;

    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
};

struct UIButton {
    glm::vec2 position;
    glm::vec2 size;
    glm::vec4 color;
    glm::vec4 hoverColor;
    std::string text;
    int id;
};

class UIRenderSystem {
   public:
    UIRenderSystem(PveDevice &device, VkRenderPass renderPass);
    ~UIRenderSystem();

    UIRenderSystem(const UIRenderSystem &) = delete;
    UIRenderSystem &operator=(const UIRenderSystem &) = delete;

    void renderGameObjects(FrameInfo &frameInfo, const std::vector<UIButton> &buttons, int mouseX, int mouseY);
    void renderText(FrameInfo &frameInfo, const std::string &text, glm::vec2 position, glm::vec4 color);

   private:
    void createPipelineLayout();
    void createPipeline(VkRenderPass renderPass);
    void ensureBufferCapacity(VkDeviceSize requiredVertexSize, VkDeviceSize requiredIndexSize);

    PveDevice &pveDevice;
    std::unique_ptr<PvePipeline> pvePipeline;
    VkPipelineLayout pipelineLayout;
    
    // Persistent buffers for UI rendering (reused each frame)
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    VkDeviceSize vertexBufferSize;
    VkDeviceSize indexBufferSize;
};

}  // namespace pve
