cmake_minimum_required(VERSION 3.20)
project(Vulkan_Base_Demos LANGUAGES CXX)

# ========== 基础编译配置 ==========
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 统一输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ========== 查找 Vulkan SDK ==========
find_package(Vulkan REQUIRED)
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found! Please install Vulkan SDK and add it to PATH.")
endif()

# ========== 第三方库配置（FetchContent） ==========
include(FetchContent)

# glm 纯头文件
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
# glfw 源码编译
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glm glfw)

# MSVC 运行库统一
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "$<IF:$<CONFIG:Debug>,MultiThreadedDebugDLL,MultiThreadedDLL>")
endif()

# ========== 收集源码文件 ==========
file(GLOB_RECURSE SRC_FILES
    "${PROJECT_SOURCE_DIR}/src/*.h"
    "${PROJECT_SOURCE_DIR}/src/*.hpp"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

# 收集着色器文件（仅用于VS工程浏览，不参与C++编译）
file(GLOB_RECURSE SHADER_FILES
    "${PROJECT_SOURCE_DIR}/src/demos/*/shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/src/demos/*/shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/src/demos/*/shaders/*.comp"
    "${PROJECT_SOURCE_DIR}/src/demos/*/shaders/*.spv"
)

# 收集资源文件
file(GLOB_RECURSE ASSET_FILES
    "${PROJECT_SOURCE_DIR}/assets/*"
)

# ========== 生成可执行文件 ==========
add_executable(${PROJECT_NAME}
    ${SRC_FILES}
    ${SHADER_FILES}
    ${ASSET_FILES}
 "src/application.h" "src/application.cpp")

# 拷贝资源
add_custom_command(TARGET Vulkan_Base_Demos POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:Vulkan_Base_Demos>/assets"
    COMMENT "Copying assets to output directory"
)

add_custom_command(TARGET Vulkan_Base_Demos POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/src/demos/model_raster/shaders"
        "$<TARGET_FILE_DIR:Vulkan_Base_Demos>/shaders"
    COMMENT "Copying model raster shaders"
)

add_custom_command(TARGET Vulkan_Base_Demos POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/src/demos/particle_system/shaders"
        "$<TARGET_FILE_DIR:Vulkan_Base_Demos>/shaders"
    COMMENT "Copying particle system shaders"
)

# ========== 头文件搜索路径【重点修复】 ==========
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${glm_SOURCE_DIR}
    ${glfw_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/thirdparty/stb
    ${PROJECT_SOURCE_DIR}/thirdparty/tinyobjloader
)

# ========== 链接库 ==========
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    Vulkan::Vulkan
    user32.lib
    gdi32.lib
    shell32.lib
)

# ========== VS 工程筛选器分组 ==========
source_group(TREE "${PROJECT_SOURCE_DIR}/src" PREFIX "Source" FILES ${SRC_FILES})
source_group(TREE "${PROJECT_SOURCE_DIR}/src/demos" PREFIX "Shaders" FILES ${SHADER_FILES})
source_group(TREE "${PROJECT_SOURCE_DIR}" PREFIX "Assets" FILES ${ASSET_FILES})

# ========== 设置 VS 启动项目 ==========
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

set_source_files_properties(${ASSET_FILES}  PROPERTIES HEADER_FILE_ONLY TRUE)

# ========== 调试打印 ==========
message(STATUS "===== CMAKE DEBUG INFO =====")
message(STATUS "Vulkan_FOUND: ${Vulkan_FOUND}")
message(STATUS "Vulkan_INCLUDE_DIR: ${Vulkan_INCLUDE_DIR}")
message(STATUS "Vulkan_INCLUDE_DIRS: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan_LIBRARY: ${Vulkan_LIBRARY}")
message(STATUS "Vulkan_LIBRARIES: ${Vulkan_LIBRARIES}")
message(STATUS "Vulkan_VERSION: ${Vulkan_VERSION}")
message(STATUS "VULKAN_SDK ENV: $ENV{VULKAN_SDK}")
message(STATUS "glm_SOURCE_DIR: ${glm_SOURCE_DIR}")
message(STATUS "glfw_SOURCE_DIR: ${glfw_SOURCE_DIR}")
message(STATUS "============================")

get_target_property(VK_TARGET_INC Vulkan::Vulkan INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "Vulkan::Vulkan INTERFACE_INCLUDE_DIRECTORIES = ${VK_TARGET_INC}")
message(STATUS "============================")
