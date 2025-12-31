#include "vulkan_layer/vulkan_flexible_shader_buffer.hpp"
#include "vulkan_layer/vulkan_object_creation_utils.hpp"
#include "engine_layer/debug.hpp"
#include "vulkan_layer/fsb_object.hpp"
#include "vulkan_layer/json/json.hpp"
#include "vulkan_layer/vulkan_buffer.hpp"
#include "vulkan_layer/vulkan_constants.hpp"
#include "vulkan_layer/vulkan_device.hpp"
#include <fstream>
#include <glm/glm.hpp>
#include <map>
#include <string>
#include <typeinfo>

using json = nlohmann::json;


std::vector<uint8_t> padFSBData(std::vector<FSBObject> ubos,
                                VkDeviceSize alignedSize, VkDeviceSize size) {
  std::vector<uint8_t> paddedData(alignedSize * ubos.size(),
                                  0);  // zero-initialized
  for (size_t i = 0; i < ubos.size(); ++i) {
    uint8_t* current = paddedData.data() + i * alignedSize;
    for (auto& kv : ubos[i]) {
      VkDeviceSize attributeSize = getStd140Size(kv.second);
      std::memcpy(current, &kv.second, attributeSize);
      current += attributeSize;
    }
  }
  return paddedData;
}

const FSBObject VulkanFSB::getDefaultObject() const { return prototype; }
VulkanFSB::VulkanFSB(const VulkanDevice& device, std::string jsonFile,
                     int uboSetNumber, VkDeviceSize alignment, std::string name)
    : alignment(alignment) {
  std::ifstream file(jsonFile);
  if (!file) {
    throw std::runtime_error("Could not load json file " + jsonFile + "!");
  }
  json j;
  file >> j;
  bool foundSet = false;
  bool foundType = false;
  std::string uboTypeName;
  for (auto& ubos : j["ubos"]) {
    if (ubos["set"] == uboSetNumber) {
      foundSet = true;
      uboTypeName = ubos["name"];
      for (auto& type : j["types"]) {
        if (type["name"] == uboTypeName) {
          foundType = true;
          for (auto& member : type["members"]) {
            prototype[member["name"]] = createDefaultUniform(member["type"]);
          }
          computeObjectSize();

          buffer = new VulkanBuffer(device, VulkanBufferType::Uniform,
                                    MAX_OBJECTS_UB * objectSize, nullptr, true,
                                    objectAlignedSize, name);
        }
      }
    }
  }
  if (!foundSet) {
    Debug::LogWarning("Shader " + jsonFile + " does not have a set numbered " +
                      std::to_string(uboSetNumber));
  } else if (!foundType) {
    Debug::LogWarning("Couldn't find ubo type" + uboTypeName + " in shader " +
                      jsonFile);
  }
}
void VulkanFSB::computeObjectSize() {
  VkDeviceSize size = 0;

  for (auto& kv : prototype) {
    size += getStd140Size(kv.second);
  }
  objectAlignedSize = (size + alignment - 1) & ~(alignment - 1);
  objectSize = size;
}
int VulkanFSB::addObject() {
  objects.push_back(FSBObject(prototype));
  return objects.size() - 1;
}

int VulkanFSB::addObject(const FSBObject& object) {
  int index = addObject();
  for (auto& kv : object) {
    setObjectAttribute(index, kv.first, kv.second);
  }
  return index;
}
void VulkanFSB::setObjectAttribute(int objectIndex,
                                   const std::string& attributeName,
                                   const UniformVariant& value) {
  if (!objects[objectIndex].contains(attributeName)) {
    throw std::runtime_error("Attribute not found: " + attributeName);
  }

  const UniformVariant& protoValue = prototype[attributeName];

  if (protoValue.index() != value.index()) {
    throw std::runtime_error("Attribute '" + attributeName + "' should have type " +
                             uniformVariantTypeName(protoValue) +
                             " but got " + uniformVariantTypeName(value) +
                             " instead!");
  }

  objects[objectIndex][attributeName] = value;
}

void VulkanFSB::pushToGPU() {
  if (objects.size() == 0) {
    return;
  }
  std::vector<uint8_t> paddedData =
      padFSBData(objects, objectAlignedSize, objectSize);
  buffer->update(paddedData.data(), paddedData.size(), 0);
}

void VulkanFSB::clear() { objects.clear(); }
void VulkanFSB::destroy() {
  buffer->destroy();
  delete buffer;
}
VkDeviceSize VulkanFSB::getObjectSize() { return objectSize; }
VulkanBuffer& VulkanFSB::getBuffer() const { return *buffer; }

void VulkanFSB::debugPrintPrototype() const {
  std::cout << "=== VulkanFSB Prototype ===\n";
  debugPrintFSBObject(prototype);
}

void VulkanFSB::debugPrintObject(int index) const {
  if (index < 0 || index >= (int)objects.size())
    throw std::runtime_error("Invalid object index");

  std::cout << "=== VulkanFSB Object " << index << " ===\n";
  debugPrintFSBObject(objects[index]);
}
bool VulkanFSB::isEmpty() { return objects.size() == 0; }
