#!/bin/bash
# RKLLM HTTP Server 初始化脚本
# 自动初始化子模块和创建必要目录

set -e  # 遇到错误时退出

echo "=== RKLLM HTTP Server 初始化 ==="

# 检查是否在git仓库中
if [ ! -d ".git" ]; then
    echo "错误：当前目录不是git仓库"
    exit 1
fi

# 1. 初始化子模块
echo "1. 初始化子模块..."
if [ -f ".gitmodules" ]; then
    echo "  找到.gitmodules文件，初始化子模块..."
    git submodule update --init --recursive
    echo "  子模块初始化完成"
else
    echo "  警告：未找到.gitmodules文件"
fi

# 2. 创建必要的目录
echo ""
echo "2. 创建必要的目录..."

# 创建model目录（如果不存在）
if [ ! -d "model" ]; then
    echo "  创建model目录..."
    mkdir -p model
    echo "  创建占位符模型文件..."
    echo "PLACEHOLDER MODEL FILE - Replace with your actual .rkllm model file" > model/Qwen2-1___5B-Instruct_W8A8_RK3588.rkllm
    echo "  创建README说明文件..."
    echo "This directory should contain your .rkllm model files." > model/README.txt
    echo "  model目录创建完成"
else
    echo "  model目录已存在"
fi

# 3. 检查lib目录中的库文件
echo ""
echo "3. 检查必要的库文件..."
if [ ! -f "lib/librkllmrt.so" ]; then
    echo "  警告：未找到 lib/librkllmrt.so"
    echo "  请将RKLLM运行时库文件放入 lib/ 目录"
fi

# 4. 检查include目录中的头文件
if [ ! -f "include/rkllm.h" ]; then
    echo "  警告：未找到 include/rkllm.h"
    echo "  请将RKLLM头文件放入 include/ 目录"
fi

echo ""
echo "=== 初始化完成 ==="
echo ""
echo "下一步："
echo "1. 确保你有正确的模型文件（.rkllm格式）"
echo "2. 将模型文件放入 model/ 目录，替换占位符文件"
echo "3. 运行编译命令："
echo "   mkdir build && cd build && cmake .. && make"
echo ""
echo "更多信息请参考 README.md"