#pragma once

#include <memory>
#include <vector>

#include "pve_descriptors.hpp"
#include "pve_device.hpp"
#include "pve_game_object.hpp"
#include "pve_renderer.hpp"
#include "pve_window.hpp"

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
    PveGameObject::Map gameObjects;
};
}  // namespace pve
