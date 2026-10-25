# Vulkan Base Demos

基于 **C++17** 和 **Vulkan API** 的现代化图形渲染框架与 Demo 集合。本项目专注于探索 Vulkan 底层渲染管线的搭建，以及计算管线与图形管线的跨队列协同，包含两个独立的高级渲染 Demo：**模型光栅化渲染器** 与 **GPU 计算粒子系统**。

> 💡![model](.\image\model.png)
> ![particle](.\image\particle.png)
>
> 完整演示视频：
> [基于Vulkan实现的双demo项目展示视频_哔哩哔哩_bilibili](https://www.bilibili.com/video/BV1cvH56wE3M/?vd_source=c8ced3ca81afa79af3b54a1a15777e46)

## 🛠 核心技术栈
*   **底层图形接口**：Vulkan 1.3+ (开启了 Khronos Validation Layer 校验，零警告)
*   **编程语言**：C++17 (广泛使用 `std::filesystem` 处理资源，智能指针管理生命周期)
*   **数学库**：GLM
*   **窗口系统**：GLFW 3.3+
*   **构建工具**：CMake

## ✨ 项目核心亮点

### 通用底层架构 (Common Layer)
采用模块化解耦设计，将 Vulkan 初始化、资源管理和生命周期逻辑从具体的业务逻辑中剥离：
*   **VulkanContext**：封装实例创建、物理设备特性筛选、逻辑设备与多队列生成。
*   **SwapchainManager**：负责交换链创建、图像视图生成，以及窗口大小改变时的自动重建。
*   **RenderPassManager**：管理渲染通道（RenderPass）、多重采样（MSAA）颜色资源、深度/模板缓冲（Depth/Stencil）及帧缓冲（Framebuffer）。
*   **CommandManager**：统一管理命令池与命令缓冲的分配与释放。
*   **SyncManager**：封装多飞行帧（in-flight frame）架构下的栅栏（Fence）与信号量（Semaphore）。
*   **ShaderUtils**：着色器 SPIR-V 字节码加载与 `VkShaderModule` 创建工具。

### Demo 1：模型光栅化渲染 (Model Rasterizer)
*   **模型加载**：集成 `tinyobjloader` 与 `stb_image`，支持带纹理的 OBJ 模型加载与顶点去重。
*   **材质渲染**：自动生成 Mipmap，开启各向异性过滤（Anisotropic Filtering），提升倾斜视角画质。
*   **相机控制**：实现透视投影与 LookAt 视图矩阵，支持 Alpha 混合与背面剔除。

### Demo 2：计算管线粒子系统 (Compute Particle System)
*   **GPU 驱动计算**：使用 Compute Shader 进行 256*256 规模粒子的物理运动模拟。
*   **乒乓缓冲（Ping-Pong SSBO）**：采用双缓冲机制，彻底分离计算管线的读写操作，避免数据竞争。
*   **跨队列同步**：使用信号量（Semaphore）与内存屏障（Memory Barrier）保证计算队列与图形队列之间的数据可见性，实现软粒子点精灵渲染。

## 📂 工程目录结构
```text
Vulkan_Base_Demos/
├── src/
│   ├── common/                  # 公共基础设施层
│   │   ├── VulkanContext.*      # 实例、设备、队列、表面管理
│   │   ├── SwapchainManager.*   # 交换链与窗口重建
│   │   ├── RenderPassManager.*  # 渲染通道、MSAA、深度模板、帧缓冲
│   │   ├── CommandManager.*     # 命令池与缓冲
│   │   ├── SyncManager.*        # 栅栏与信号量
│   │   └── ShaderUtils.*        # 着色器加载工具
│   ├── demos/                   # 独立 Demo 业务逻辑
│   │   ├── model_raster/        # 模型渲染 (含 Shaders/Assets)
│   │   └── particle_system/     # 粒子系统 (含 Compute Shader)
│   ├── Application.*            # 应用工厂与主干
│   └── main.cpp                 # 程序入口
├── assets/                      # 全局静态资源
└── thirdparty/                  # 第三方轻量级库 (tinyobj, stb)
```

## 🚀 构建与运行指南 (Build & Run)

### 1. 环境依赖
*   [Vulkan SDK](https://vulkan.lunarg.com/) (1.3 或更高版本)
*   CMake (3.10+)
*   Visual Studio 2022 (MSVC 编译器)

### 2. 编译项目
在项目根目录下打开终端（PowerShell / CMD）：

```bash
# 生成构建系统
cmake -S . -B build
# 编译 Release 或 Debug 版本
cmake --build build --config Release
```
*(注：使用 CMake 的 `POST_BUILD` 指令，编译成功后会自动将 `assets` 和 `shaders` 拷贝至输出目录，保证相对路径正确)*

### 3. 运行方式 ⚠️
**注意：** 本程序为多 Demo 框架，默认双击不会启动，请打开命令行，通过参数指定需要运行的 Demo。

进入输出目录（如 `build/bin/Release`），执行以下命令：

```bash
# 运行模型渲染 Demo
./Vulkan_Base_Demos.exe model

# 运行计算管线粒子系统 Demo
./Vulkan_Base_Demos.exe particle
```

---
## 💡 技术难点攻克（工程复盘）
*   **管线状态污染**：粒子渲染与模型渲染的顶点格式、图元拓扑不一致，早期共用管线导致数据错乱。**解决**：分离管线创建逻辑，为粒子系统单独创建基于点精灵（Point Sprite）的图形管线。
*   **跨队列数据可见性**：Compute Shader 算完粒子位置后，Graphics 管线读取到旧数据。**解决**：在记录命令缓冲时，于计算管线与图形管线之间插入 `vkCmdPipelineBarrier`，确保内存写入对后续顶点输入阶段可见。
*   **资源路径解耦**：早期使用相对路径导致在外部终端启动时找不到模型资源。**解决**：使用 `std::filesystem` 配合 Windows API `GetModuleFileNameW`，以 `.exe` 所在目录为基准寻找资源，实现 "目录即跑"（无需配置工作目录）。