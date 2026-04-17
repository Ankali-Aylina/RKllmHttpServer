#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <crow.h>
#include <nlohmann/json.hpp>
#include <string>
#include <atomic>

// HTTP服务器管理类
class HttpServer {
public:
    // 构造函数
    HttpServer(int port = 8080);
    
    // 启动服务器
    void start();
    
    // 停止服务器
    void stop();
    
    // 设置关机标志
    void setShutdownFlag(bool flag) { m_shutdown_flag = flag; }
    
    // 获取应用指针（用于信号处理）
    crow::SimpleApp* getApp() { return &m_app; }
    
    // 注册路由
    void setupRoutes();
    
private:
    // 健康检查路由处理
    crow::response healthCheck();
    
    // OpenAI兼容的聊天补全接口
    crow::response chatCompletions(const crow::request& req);
    
    // 简化聊天接口
    crow::response simpleChat(const crow::request& req);
    
    crow::SimpleApp m_app;
    int m_port;
    std::atomic<bool> m_shutdown_flag{false};
};

// 信号处理函数
void setupSignalHandlers(HttpServer* server);

#endif // HTTP_SERVER_H