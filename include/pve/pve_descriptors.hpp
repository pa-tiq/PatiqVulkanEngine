#pragma once

#include "pve_device.hpp"

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace pve {

class PveDescriptorSetLayout {
   public:
    class Builder {
       public:
        Builder(PveDevice &pveDevice) : pveDevice{pveDevice} {}

        Builder &addBinding(
            uint32_t binding,
            VkDescriptorType descriptorType,
            VkShaderStageFlags stageFlags,
            uint32_t count = 1);
        std::unique_ptr<PveDescriptorSetLayout> build() const;

       private:
        PveDevice &pveDevice;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
    };

    PveDescriptorSetLayout(
        PveDevice &pveDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
    ~PveDescriptorSetLayout();
    PveDescriptorSetLayout(const PveDescriptorSetLayout &) = delete;
    PveDescriptorSetLayout &operator=(const PveDescriptorSetLayout &) = delete;

    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

   private:
    PveDevice &pveDevice;
    VkDescriptorSetLayout descriptorSetLayout;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

    friend class PveDescriptorWriter;
};

class PveDescriptorPool {
   public:
    class Builder {
       public:
        Builder(PveDevice &pveDevice) : pveDevice{pveDevice} {}

        Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count);
        Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
        Builder &setMaxSets(uint32_t count);
        std::unique_ptr<PveDescriptorPool> build() const;

       private:
        PveDevice &pveDevice;
        std::vector<VkDescriptorPoolSize> poolSizes{};
        uint32_t maxSets = 1000;
        VkDescriptorPoolCreateFlags poolFlags = 0;
    };

    PveDescriptorPool(
        PveDevice &pveDevice,
        uint32_t maxSets,
        VkDescriptorPoolCreateFlags poolFlags,
        const std::vector<VkDescriptorPoolSize> &poolSizes);
    ~PveDescriptorPool();
    PveDescriptorPool(const PveDescriptorPool &) = delete;
    PveDescriptorPool &operator=(const PveDescriptorPool &) = delete;

    bool allocateDescriptor(
        const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const;

    void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;

    void resetPool();

   private:
    PveDevice &pveDevice;
    VkDescriptorPool descriptorPool;

    friend class PveDescriptorWriter;
};

class PveDescriptorWriter {
   public:
    PveDescriptorWriter(PveDescriptorSetLayout &setLayout, PveDescriptorPool &pool);

    PveDescriptorWriter &writeBuffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo);
    PveDescriptorWriter &writeImage(uint32_t binding, VkDescriptorImageInfo *imageInfo);

    bool build(VkDescriptorSet &set);
    void overwrite(VkDescriptorSet &set);

   private:
    PveDescriptorSetLayout &setLayout;
    PveDescriptorPool &pool;
    std::vector<VkWriteDescriptorSet> writes;
};

}  // namespace pve