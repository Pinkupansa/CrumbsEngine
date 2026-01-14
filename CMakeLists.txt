cmake_minimum_required(VERSION 3.15)
project(vulkan_test CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------
# Compiler / Sanitizers Setup
# -------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Use Debug-friendly optimization for sanitizers
    set(CMAKE_BUILD_TYPE Debug)
    
    # Sanitizers: Undefined + Address
    set(SANITIZERS "-fsanitize=undefined,address")
    
    # Compile flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZERS} -fno-omit-frame-pointer -fno-sanitize-recover=all -fno-optimize-sibling-calls -g")
    
    # Linker flags
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZERS}")
endif()

# -------------------------------
# Vulkan + GLFW
# -------------------------------
find_package(Vulkan REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)

# Find Assimp
find_package(assimp REQUIRED)

add_subdirectory(external/imgui)
# -------------------------------
# Optional: ccache
# -------------------------------
find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER   ccache)
    set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
endif()

# -------------------------------
# Sources & Executable
# -------------------------------
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h" "include/*.hpp")
add_executable(vulkan_test ${SOURCES} ${HEADERS})

target_link_libraries(vulkan_test PRIVATE imgui)

target_include_directories(vulkan_test PRIVATE
    include
    ${Vulkan_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
)

target_link_directories(vulkan_test PRIVATE ${GLFW_LIBRARY_DIRS} ${ASSIMP_INCLUDE_DIRS})
target_link_libraries(vulkan_test PRIVATE
    ${Vulkan_LIBRARIES}
    ${GLFW_LIBRARIES}
    ${ASSIMP_LIBRARIES}
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
    "-framework CoreFoundation"
)

# -------------------------------
# Shader compilation
# -------------------------------
set(SHADER_SRC_DIR "${CMAKE_SOURCE_DIR}/src/shaders")
set(SHADER_OUT_DIR "${CMAKE_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

file(GLOB FRAG_SHADERS "${SHADER_SRC_DIR}/*.frag.glsl")
file(GLOB VERT_SHADERS "${SHADER_SRC_DIR}/*.vert.glsl")

set(SPIRV_BINARIES "")

# Vertex shaders
foreach(SHADER ${VERT_SHADERS})
    get_filename_component(BASENAME ${SHADER} NAME_WE)
    set(SPIRV "${SHADER_OUT_DIR}/${BASENAME}.vert.spv")

    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND glslc -fshader-stage=vertex "${SHADER}" -o "${SPIRV}" 
        DEPENDS "${SHADER}"
        COMMENT "Compiling vertex shader ${BASENAME}"
        VERBATIM
    )

    list(APPEND SPIRV_BINARIES ${SPIRV})
endforeach()

# Fragment shaders
foreach(SHADER ${FRAG_SHADERS})
    get_filename_component(BASENAME ${SHADER} NAME_WE)
    set(SPIRV "${SHADER_OUT_DIR}/${BASENAME}.frag.spv")
    set(REFLECT "${SHADER_OUT_DIR}/${BASENAME}.frag.json")

    add_custom_command(
        OUTPUT ${SPIRV} ${REFLECT}
        COMMAND glslc -fshader-stage=fragment "${SHADER}" -o "${SPIRV}"
        COMMAND spirv-cross "${SPIRV}" --reflect --output "${REFLECT}"
        DEPENDS "${SHADER}"
        COMMENT "Compiling + reflecting fragment shader ${BASENAME}"
        VERBATIM
    )

    list(APPEND SPIRV_BINARIES ${SPIRV})
endforeach()

# Custom target that builds all shaders
add_custom_target(shaders ALL DEPENDS ${SPIRV_BINARIES})

# Make executable depend on shaders
add_dependencies(vulkan_test shaders)

# -------------------------------
# Precompiled headers
# -------------------------------
target_precompile_headers(vulkan_test PRIVATE
    <vulkan/vulkan.h>
    <GLFW/glfw3.h>
    <vector>
    <string>
    <unordered_map>
)

# -------------------------------
# OpenMP
# -------------------------------
set(OpenMP_ROOT "/opt/homebrew/opt/libomp")
find_package(OpenMP REQUIRED)
target_link_libraries(vulkan_test PRIVATE OpenMP::OpenMP_CXX)

# -------------------------------
# Additional safety compile flags
# -------------------------------
target_compile_options(vulkan_test PRIVATE

    -Wshadow
    -Wuninitialized
    -Wunused
    -fsanitize=float-cast-overflow,float-divide-by-zero
    -fsanitize=signed-integer-overflow
    -O3
)
