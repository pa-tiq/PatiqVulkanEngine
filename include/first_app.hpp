#pragma once

#include <memory>
#include <vector>

#include "pve/pve_descriptors.hpp"
#include "pve/pve_device.hpp"
#include "pve/pve_game_object.hpp"
#include "pve/pve_game_state.hpp"
#include "pve/pve_i18n.hpp"
#include "pve/pve_renderer.hpp"
#include "pve/pve_window.hpp"
#include "systems/shadow_map_system.hpp"
#include "systems/ui_render_system.hpp"

namespace pve {
class FirstApp {
   public:
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 600;

    FirstApp();
    ~FirstApp();

    FirstApp(const FirstApp &) = delete;
    FirstApp &operator=(const FirstApp &) = delete;

    void run();

   private:
    void loadGameObjects();
    void handleInput(GLFWwindow* window, float frameTime);
    void renderMenu(VkCommandBuffer commandBuffer);
    void renderPauseModal(VkCommandBuffer commandBuffer);

    PveWindow pveWindow{WIDTH, HEIGHT, "Hello Vulkan!"};
    PveDevice pveDevice{pveWindow};
    PveRenderer pveRenderer{pveWindow, pveDevice};

    std::unique_ptr<PveDescriptorPool> globalPool{};
    std::unique_ptr<ShadowMapSystem> shadowMapSystem;
    PveGameObject::Map gameObjects;
    std::unique_ptr<PveGameObject> viewerObject;
    std::unique_ptr<UIRenderSystem> uiRenderSystem;

    GameStateManager& gameState;
    I18n& i18n;

    double mouseX = 0.0;
    double mouseY = 0.0;
    int selectedMenuIndex = 0;

    static constexpr uint32_t SHADOW_MAP_SIZE = 2048;
};
}  // namespace pve
