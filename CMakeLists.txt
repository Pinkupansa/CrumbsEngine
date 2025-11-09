cmake_minimum_required(VERSION 3.15)
project(vulkan_test CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Vulkan + GLFW
find_package(Vulkan REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)

# Find Assimp
find_package(assimp REQUIRED)

# Sources
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h" "include/*.hpp")
add_executable(vulkan_test ${SOURCES} ${HEADERS})

target_include_directories(vulkan_test PRIVATE
    include/vulkan_layer
    include/engine_layer
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

file(GLOB FRAG_SHADERS "${SHADER_SRC_DIR}/*.frag.glsl")
file(GLOB VERT_SHADERS "${SHADER_SRC_DIR}/*.vert.glsl")
# Create output folder
file(MAKE_DIRECTORY ${SHADER_OUT_DIR})
message(STATUS "Found shaders: ${SHADER_SRC_DIR}")
set(SPIRV_BINARIES "")
foreach(SHADER ${VERT_SHADERS})
    get_filename_component(BASENAME ${SHADER} NAME_WE)  # name without extension
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

foreach(SHADER ${FRAG_SHADERS})
    get_filename_component(BASENAME ${SHADER} NAME_WE)
    set(SPIRV "${SHADER_OUT_DIR}/${BASENAME}.frag.spv")

    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND glslc -fshader-stage=fragment "${SHADER}" -o "${SPIRV}" 
        DEPENDS "${SHADER}"
        COMMENT "Compiling fragment shader ${BASENAME}"
        VERBATIM
    )

    list(APPEND SPIRV_BINARIES ${SPIRV})
endforeach()
# Custom target that builds all shaders
add_custom_target(shaders ALL DEPENDS ${SPIRV_BINARIES})

# Make executable depend on shaders
add_dependencies(vulkan_test shaders)

target_compile_options(vulkan_test PRIVATE
        -fsanitize=address
        -fno-omit-frame-pointer
        -g
    )
    target_link_options(vulkan_test PRIVATE
        -fsanitize=address
    )