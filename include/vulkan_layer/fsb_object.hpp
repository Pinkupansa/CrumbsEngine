#pragma once 
#include <map>
#include <glm/glm.hpp>


// when adding a possible type, add the default in createDefaultUniform, and the type name in uniformVariantTypeName
using UniformVariant =
std::variant<float, bool, uint32_t, int, glm::vec2, glm::vec3, glm::vec4, glm::mat2, glm::mat3, glm::mat4>;

UniformVariant createDefaultUniform (const std::string& typeName) {
    if (typeName == "float")
        return 0.0f;
    if (typeName == "bool")
        return true;
    if (typeName == "uint")
        return uint32_t (0);
    if (typeName == "int")
        return int (-1);
    if (typeName == "vec2")
        return glm::vec2 (1.0f);
    if (typeName == "vec3")
        return glm::vec3 (0.0f);
    if (typeName == "vec4")
        return glm::vec4 (0.0f);
    if (typeName == "mat2")
        return glm::mat2 (1.0f);
    if (typeName == "mat3")
        return glm::mat3 (1.0f);
    if (typeName == "mat4")
        return glm::mat4 (1.0f);

    throw std::runtime_error ("Unknown type: " + typeName);
}


const char* uniformVariantTypeName (const UniformVariant& v) {
    switch (v.index ()) {
    case 0: return "float";
    case 1: return "bool";
    case 2: return "uint";
    case 3: return "int";
    case 4: return "vec2";
    case 5: return "vec3";
    case 6: return "vec4";
    case 7: return "mat2";
    case 8: return "mat3";
    case 9: return "mat4";
    default: return "unknown";
    }
}
inline uint64_t getStd140Alignment(const UniformVariant& v) {
    switch (v.index()) {
        case 0: case 1: case 2: case 3: return 4;      // float, bool, int, uint
        case 4: return 8;                               // vec2
        case 5: case 6: return 16;                      // vec3, vec4
        case 7: case 8: case 9: return 16;              // mat2, mat3, mat4 (columns aligned to vec4)
        default: throw std::runtime_error("Unknown UniformVariant type");
    }
}

inline uint64_t getStd140Size(const UniformVariant& v) {
    switch (v.index()) {
        case 0: case 1: case 2: case 3: return 4;
        case 4: return 8;
        case 5: return 16;          // vec3 padded to 16
        case 6: return 16;          // vec4
        case 7: return 16 * 2;      // mat2: 2 vec4 columns
        case 8: return 16 * 3;      // mat3: 3 vec4 columns
        case 9: return 16 * 4;      // mat4: 4 vec4 columns
        default: throw std::runtime_error("Unknown UniformVariant type");
    }
}

inline void printUniformVariant(const UniformVariant& v) {
    std::visit([](auto&& val) {
        using T = std::decay_t<decltype(val)>;

        if constexpr (std::is_same_v<T, float>)
            std::cout << val;
        else if constexpr (std::is_same_v<T, bool>)
            std::cout << (val ? "true" : "false");
        else if constexpr (std::is_same_v<T, uint32_t>)
            std::cout << val;
        else if constexpr (std::is_same_v<T, int>)
            std::cout << val;
        else if constexpr (std::is_same_v<T, glm::vec2>)
            std::cout << "(" << val.x << ", " << val.y << ")";
        else if constexpr (std::is_same_v<T, glm::vec3>)
            std::cout << "(" << val.x << ", " << val.y << ", " << val.z << ")";
        else if constexpr (std::is_same_v<T, glm::vec4>)
            std::cout << "(" << val.x << ", " << val.y << ", " << val.z << ", " << val.w << ")";
        else if constexpr (std::is_same_v<T, glm::mat2>)
            std::cout << "mat2";
        else if constexpr (std::is_same_v<T, glm::mat3>)
            std::cout << "mat3";
        else if constexpr (std::is_same_v<T, glm::mat4>)
            std::cout << "mat4";
    }, v);
}

#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <stdexcept>

template<typename T>
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
        if (it == indexMap.end())
            throw std::out_of_range("Key not found: " + key);
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

inline void debugPrintFSBObject(const FSBObject& obj) {
    std::cout << "FSBObject {\n";

    uint64_t offset = 0;

    for (const auto& [name, value] : obj) {
        uint64_t align = getStd140Alignment(value);
        if (offset % align != 0)
            offset += align - (offset % align);

        std::cout << "  " << name << " : "
                  << uniformVariantTypeName(value)
                  << " | align=" << align
                  << " | size=" << getStd140Size(value)
                  << " | offset=" << offset
                  << " | value=";

        printUniformVariant(value);
        std::cout << "\n";

        offset += getStd140Size(value);
    }

    if (offset % 16 != 0)
        offset += 16 - (offset % 16);

    std::cout << "}  // total std140 size = " << offset << " bytes\n";
}
