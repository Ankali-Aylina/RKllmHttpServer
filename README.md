# RKLLM HTTP Server

基于 C++ 的高性能 HTTP 服务，为 Rockchip RK3588（如 Orange Pi 5 Plus）的 NPU 提供 OpenAI 兼容的 LLM 推理接口。  
通过 Crow Web 框架对外提供 REST API，直接调用 RKLLM C++ 运行时库，实现低延迟、高吞吐的本地大模型服务。

## 功能特性

- ✅ 原生 C++ 实现，无 Python 中间层开销
- ✅ 支持 OpenAI 兼容的 `/v1/chat/completions` 接口
- ✅ 支持简化接口 `/chat`（直接传递 `prompt` 字段）
- ✅ 充分利用 RK3588 的 3 个 NPU 核心
- ✅ 多线程并发处理（需注意模型实例的线程安全性）
- ✅ 可动态配置生成参数（temperature, top_p, top_k 等）

## 项目结构（模块化设计）

重构后的项目采用模块化设计，便于维护和扩展：

```
rkllmHttpServer/
├── CMakeLists.txt          # 构建配置
├── main.cpp               # 主程序入口
├── llm_model.h/cpp        # 模型管理模块（单例模式）
├── http_server.h/cpp      # HTTP服务器模块
├── include/
│   └── rkllm.h           # RKLLM API头文件
├── lib/
│   └── librkllmrt.so     # RKLLM运行时库
├── model/
│   └── Qwen2-1___5B-Instruct_W8A8_RK3588.rkllm  # 模型文件
├── Crow/                  # Crow HTTP库
└── json/                  # nlohmann/json库
```

### 模块说明

- **LLM模型模块**：单例模式管理RKLLM模型实例，提供线程安全的推理接口
- **HTTP服务器模块**：管理HTTP服务器生命周期，处理路由和请求
- **主程序**：协调模块初始化，提供程序入口点

## 环境要求

- **硬件**：Orange Pi 5 Plus 或其他 RK3588 设备
- **系统**：Armbian / Ubuntu 22.04+ （内核包含 NPU 驱动）
- **NPU 驱动**：已安装 `rknpu2` 驱动（版本 ≥ 0.9.8）
- **RKLLM 运行时**：`librkllmrt.so` 库文件已放入 `lib/` 文件夹
- **模型文件**：已通过 `rkllm-toolkit` 转换好的 `.rkllm` 模型放入 `model/` 文件夹

> 如果你还没有配置好 NPU 环境和 RKLLM 运行时，请先参考 [RKLLM 官方文档](https://github.com/airockchip/rknn-llm) 或相关社区教程完成板端环境搭建。

## 编译与运行

### 1. 安装系统依赖

```bash
sudo apt update
sudo apt install -y build-essential cmake git libasio-dev libboost-system-dev
```

### 2. 克隆项目并初始化子模块

```bash
# 克隆项目（包含子模块）
git clone --recursive https://github.com/Ankali-Aylina/rkllmHttpServer.git
cd rkllmHttpServer
```

或者，如果已经克隆了项目但没有使用 `--recursive` 参数：

```bash
git clone https://github.com/Ankali-Aylina/rkllmHttpServer.git
cd rkllmHttpServer
git submodule update --init --recursive
```

### 2.1 使用初始化脚本（推荐）

如果遇到子模块初始化问题或需要自动创建目录结构，可以使用提供的初始化脚本：

**Windows 用户：**

```bash
init.bat
```

**Linux/macOS 用户：**

```bash
chmod +x init.sh
./init.sh
```

初始化脚本会自动：

1. 初始化所有子模块
2. 创建必要的目录结构（包括 `model/` 目录）
3. 创建占位符模型文件
4. 检查必要的库文件和头文件

### 3. 准备模型和库文件

将你的 `.rkllm` 模型文件放入 `model/` 文件夹，将 `librkllmrt.so` 库文件放入 `lib/` 文件夹。

项目已包含：

- `model/Qwen2-1___5B-Instruct_W8A8_RK3588.rkllm` - **占位符文件**（需要替换为实际模型）
- `lib/librkllmrt.so` - RKLLM运行时库
- `include/rkllm.h` - RKLLM API头文件

**重要提示**：`model/` 目录中的 `Qwen2-1___5B-Instruct_W8A8_RK3588.rkllm` 文件是一个占位符，不包含实际的模型数据。你需要：

1. 使用 `rkllm-toolkit` 转换自己的模型为 `.rkllm` 格式
2. 将转换后的模型文件放入 `model/` 文件夹，**替换**占位符文件
3. 确保 `main.cpp` 中的模型路径与你的模型文件名匹配（默认为 `model/Qwen2-1___5B-Instruct_W8A8_RK3588.rkllm`）

如需使用其他模型，只需将新的 `.rkllm` 文件放入 `model/` 文件夹，并修改 `main.cpp` 中的模型路径。

### 4. 编译

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 5. 运行服务

```bash
./rkllm_http_server
```

如果一切正常，你会看到类似如下输出：

```text
RKLLM initialized successfully.
Starting HTTP server on http://0.0.0.0:8080
```

## API 使用示例

服务默认监听 0.0.0.0:8080，支持以下接口。

### 1. OpenAI 兼容接口

#### 请求 (/v1/chat/completions)

```bash
curl -X POST http://localhost:8080/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "messages": [
      {"role": "user", "content": "你好，请介绍一下自己"}
    ]
  }'
```

#### 响应

```json
{
  "choices": [
    {
      "finish_reason": "stop",
      "message": {
        "content": "我是一个人工智能模型，可以回答各种问题和提供帮助。我可以进行文本生成、自然语言处理、聊天等任务。",
        "role": "assistant"
      }
    }
  ]
}
```

### 2. 简化接口

#### 请求 (/chat)

```bash
curl -X POST http://localhost:8080/chat \
  -H "Content-Type: application/json" \
  -d '{"prompt": "1+1等于几？"}'
```

#### 响应

```json
{
  "response": "1+1等于2。"
}
```

### 3. 健康检查

```bash
curl http://localhost:8080/health
# 返回 "OK"
```

## 许可证

本项目采用 MIT 许可证，可自由使用和修改。

## 免责声明

本项目为开源软件，按"原样"提供，不附带任何明示或暗示的担保。使用者需自行承担使用风险。作者不对因使用本项目而导致的任何直接或间接损失负责。

本项目依赖的第三方库（RKLLM、Crow、nlohmann/json）有其各自的许可证，使用者应遵守相应许可证条款。

## 致谢

[Rockchip RKLLM](https://github.com/airockchip/rknn-llm) – NPU 推理运行时

[CrowCpp](https://github.com/CrowCpp/Crow) – 轻量级 C++ Web 框架

[nlohmann/json](https://github.com/nlohmann/json) – 现代 C++ JSON 库
