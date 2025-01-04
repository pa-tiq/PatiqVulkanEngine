#pragma once

#include <memory>
#include <vector>

#include "pve/pve_descriptors.hpp"
#include "pve/pve_device.hpp"
#include "pve/pve_game_object.hpp"
#include "pve/pve_renderer.hpp"
#include "pve/pve_window.hpp"
#include "systems/shadow_map_system.hpp"

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

    PveWindow pveWindow{WIDTH, HEIGHT, "Hello Vulkan!"};
    PveDevice pveDevice{pveWindow};
    PveRenderer pveRenderer{pveWindow, pveDevice};

    std::unique_ptr<PveDescriptorPool> globalPool{};
    std::unique_ptr<ShadowMapSystem> shadowMapSystem;
    PveGameObject::Map gameObjects;

    static constexpr uint32_t SHADOW_MAP_SIZE = 2048;
};
}  // namespace pve
