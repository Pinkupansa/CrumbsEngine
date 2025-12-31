#pragma once
#include <glm/glm.hpp>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

// when adding a possible type, add the default in createDefaultUniform, and the
// type name in uniformVariantTypeName
using UniformVariant = std::variant<float, bool, uint32_t, int, glm::vec2,
                                  glm::vec3, glm::vec4, glm::mat2, glm::mat3,
                                  glm::mat4>;

UniformVariant createDefaultUniform(const std::string& typeName);
const char* uniformVariantTypeName(const UniformVariant& v);
uint64_t getStd140Alignment(const UniformVariant& v);
uint64_t getStd140Size(const UniformVariant& v);
void printUniformVariant(const UniformVariant& v);

template <typename T>
class OrderedMap {
 public:
  // Insert a new key-value pair (throws if exists)
  void insert(const std::string& key, const T& value) {
    if (indexMap.find(key) != indexMap.end())
      throw std::runtime_error("Key already exists: " + key);
    indexMap[key] = ordered.size();
    ordered.emplace_back(key, value);
  }

  // operator[] for access or assignment
  T& operator[](const std::string& key) {
    auto it = indexMap.find(key);
    if (it == indexMap.end()) {
      // Key does not exist → insert default T
      indexMap[key] = ordered.size();
      ordered.emplace_back(key, T{});
      return ordered.back().second;
    }
    return ordered[it->second].second;
  }

  const T& operator[](const std::string& key) const {
    auto it = indexMap.find(key);
    if (it == indexMap.end()) throw std::out_of_range("Key not found: " + key);
    return ordered[it->second].second;
  }

  // Range-based for iteration
  auto begin() { return ordered.begin(); }
  auto end() { return ordered.end(); }
  auto begin() const { return ordered.begin(); }
  auto end() const { return ordered.end(); }
  auto cbegin() const { return ordered.cbegin(); }
  auto cend() const { return ordered.cend(); }

  // Check if key exists
  bool contains(const std::string& key) const {
    return indexMap.find(key) != indexMap.end();
  }

  // Get ordered elements for iteration
  const std::vector<std::pair<std::string, T>>& getOrdered() const {
    return ordered;
  }

 private:
  std::vector<std::pair<std::string, T>> ordered;
  std::unordered_map<std::string, size_t> indexMap;
};

using FSBObject = OrderedMap<UniformVariant>;

void debugPrintFSBObject(const FSBObject& obj);