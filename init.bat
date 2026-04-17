@echo off
echo === RKLLM HTTP Server 初始化 ===
echo.

REM 检查是否在git仓库中
if not exist ".git" (
    echo 错误：当前目录不是git仓库
    exit /b 1
)

REM 1. 初始化子模块
echo 1. 初始化子模块...
if exist ".gitmodules" (
    echo   找到.gitmodules文件，初始化子模块...
    git submodule update --init --recursive
    echo   子模块初始化完成
) else (
    echo   警告：未找到.gitmodules文件
)

REM 2. 创建必要的目录
echo.
echo 2. 创建必要的目录...

REM 创建model目录（如果不存在）
if not exist "model" (
    echo   创建model目录...
    mkdir model
    echo   创建占位符模型文件...
    echo PLACEHOLDER MODEL FILE - Replace with your actual .rkllm model file > model\Qwen2-1___5B-Instruct_W8A8_RK3588.rkllm
    echo   创建README说明文件...
    echo This directory should contain your .rkllm model files. > model\README.txt
    echo   model目录创建完成
) else (
    echo   model目录已存在
)

REM 3. 检查lib目录中的库文件
echo.
echo 3. 检查必要的库文件...
if not exist "lib\librkllmrt.so" (
    echo   警告：未找到 lib\librkllmrt.so
    echo   请将RKLLM运行时库文件放入 lib\ 目录
)

REM 4. 检查include目录中的头文件
if not exist "include\rkllm.h" (
    echo   警告：未找到 include\rkllm.h
    echo   请将RKLLM头文件放入 include\ 目录
)

echo.
echo === 初始化完成 ===
echo.
echo 下一步：
echo 1. 确保你有正确的模型文件（.rkllm格式）
echo 2. 将模型文件放入 model\ 目录，替换占位符文件
echo 3. 运行编译命令：
echo    mkdir build ^&^& cd build ^&^& cmake .. ^&^& make
echo.
echo 更多信息请参考 README.md
pause