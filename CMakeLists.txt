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

# ========== 第三方库配置（thirdparty 内嵌） ==========
# glm：纯头文件库，直接加 include 路径
set(GLM_DIR "${PROJECT_SOURCE_DIR}/thirdparty/glm")

# GLFW：预编译静态库（官方下载的 VC 预编译包结构）
# 注意：如果你的 glfw 库文件夹名不是 lib-vc2022，改成你实际的名字
set(GLFW_DIR "${PROJECT_SOURCE_DIR}/thirdparty/glfw")
set(GLFW_INCLUDE_DIR "${GLFW_DIR}/include")
set(GLFW_LIB "${GLFW_DIR}/lib-vc2022/glfw3.lib")
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "$<IF:$<CONFIG:Debug>,MultiThreadedDebugDLL,MultiThreadedDLL>")
endif()

# 静态链接 GLFW 需要的宏
add_compile_definitions(GLFW_STATIC)

# ========== 收集源码文件 ==========
# 递归扫描 src 下所有 .h/.cpp（common + 两个demo + main.cpp）
file(GLOB_RECURSE SRC_FILES
    "${PROJECT_SOURCE_DIR}/src/*.h"
    "${PROJECT_SOURCE_DIR}/src/*.hpp"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

# 收集着色器文件（加入工程方便浏览，不参与C++编译）
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
 "src/common/syncManager.h" "src/common/syncManager.cpp" "src/demos/particle_system/particleSystem.h" "src/demos/particle_system/particleSystem.cpp")

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
    COMMENT "Copying shaders"
)

add_custom_command(TARGET Vulkan_Base_Demos POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/src/demos/particle_system/shaders"
        "$<TARGET_FILE_DIR:Vulkan_Base_Demos>/shaders"
    COMMENT "Copying shaders"
)

# ========== 头文件搜索路径 ==========
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/src       # 源码根目录，统一用 "common/xxx.h" "demos/xxx.h" 引用
    ${GLM_DIR}
    ${GLFW_INCLUDE_DIR}
    ${Vulkan_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/thirdparty/stb
    ${PROJECT_SOURCE_DIR}/thirdparty/tinyobjloader
)

# ========== 链接库 ==========
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${GLFW_LIB}
    Vulkan::Vulkan
    # Windows 下 GLFW 静态链接依赖的系统库
    user32.lib
    gdi32.lib
    shell32.lib
)

# ========== VS 工程筛选器分组（和磁盘目录保持一致） ==========
source_group(TREE "${PROJECT_SOURCE_DIR}/src" PREFIX "Source" FILES ${SRC_FILES})
source_group(TREE "${PROJECT_SOURCE_DIR}/src/demos" PREFIX "Shaders" FILES ${SHADER_FILES})
source_group(TREE "${PROJECT_SOURCE_DIR}" PREFIX "Assets" FILES ${ASSET_FILES})

# ========== 可选：设置 VS 启动项目 ==========
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
message(STATUS "============================")

# 打印 IMPORTED target 的接口头路径（写在CMakeLists里面，不是PS）
get_target_property(VK_TARGET_INC Vulkan::Vulkan INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "Vulkan::Vulkan INTERFACE_INCLUDE_DIRECTORIES = ${VK_TARGET_INC}")
message(STATUS "============================")
