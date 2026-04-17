#include <iostream>
#include "llm_model.h"
#include "http_server.h"

int main() {
    // 初始化模型
    const char* model_path = "model/Qwen2-1___5B-Instruct_W8A8_RK3588.rkllm";
    auto& model = LLMModel::getInstance();
    
    if (!model.initialize(model_path)) {
        std::cerr << "Failed to initialize model. Exiting." << std::endl;
        return -1;
    }
    
    // 创建HTTP服务器
    HttpServer server(8080);
    
    // 设置信号处理
    setupSignalHandlers(&server);
    
    // 设置路由
    server.setupRoutes();
    
    // 启动服务器
    server.start();
    
    // 服务器停止后清理模型
    model.cleanup();
    
    std::cout << "Application exited cleanly." << std::endl;
    return 0;
}