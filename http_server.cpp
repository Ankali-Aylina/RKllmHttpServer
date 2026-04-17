#include "http_server.h"
#include "llm_model.h"
#include <iostream>
#include <csignal>
#include <atomic>

using json = nlohmann::json;

// 全局变量用于信号处理
static std::atomic<HttpServer*> g_server_ptr{nullptr};

// 信号处理函数
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        std::cout << "\nReceived shutdown signal. Stopping server..." << std::endl;
        auto* server = g_server_ptr.load();
        if (server) {
            server->setShutdownFlag(true);
            server->stop();
        }
    }
}

// 设置信号处理器
void setupSignalHandlers(HttpServer* server) {
    g_server_ptr.store(server);
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
}

// 构造函数
HttpServer::HttpServer(int port) : m_port(port) {
    // 设置日志级别为Warning以减少信息输出
    m_app.loglevel(crow::LogLevel::Warning);
}

// 启动服务器
void HttpServer::start() {
    std::cout << "Starting HTTP server on http://0.0.0.0:" << m_port << std::endl;
    m_app.port(m_port).multithreaded().run();
    std::cout << "Server stopped." << std::endl;
}

// 停止服务器
void HttpServer::stop() {
    m_app.stop();
}

// 注册路由
void HttpServer::setupRoutes() {
    // 健康检查
    CROW_ROUTE(m_app, "/health")
    ([this]() { return healthCheck(); });
    
    // OpenAI 兼容的聊天补全接口
    CROW_ROUTE(m_app, "/v1/chat/completions")
        .methods("POST"_method)([this](const crow::request &req) {
            return chatCompletions(req);
        });
    
    // 简化接口 /chat
    CROW_ROUTE(m_app, "/chat")
        .methods("POST"_method)([this](const crow::request &req) {
            return simpleChat(req);
        });
}

// 健康检查路由处理
crow::response HttpServer::healthCheck() {
    return crow::response(200, "OK");
}

// OpenAI兼容的聊天补全接口
crow::response HttpServer::chatCompletions(const crow::request &req) {
    if (req.body.empty()) {
        return crow::response(400, "Empty body");
    }
    
    json req_json;
    try {
        req_json = json::parse(req.body);
    } catch (const std::exception& e) {
        return crow::response(400, "Invalid JSON");
    }
    
    // 提取用户消息
    std::string user_prompt;
    if (req_json.contains("messages") && req_json["messages"].is_array()) {
        for (auto& msg : req_json["messages"]) {
            if (msg["role"] == "user") {
                user_prompt = msg["content"].get<std::string>();
                break;
            }
        }
    } else if (req_json.contains("prompt")) {
        user_prompt = req_json["prompt"].get<std::string>();
    } else {
        return crow::response(400, "Missing 'messages' or 'prompt' field");
    }
    
    if (user_prompt.empty()) {
        return crow::response(400, "No user prompt found");
    }
    
    // 执行推理
    auto& model = LLMModel::getInstance();
    std::string answer = model.infer(user_prompt);
    
    json resp;
    resp["choices"] = json::array();
    resp["choices"][0]["message"]["role"] = "assistant";
    resp["choices"][0]["message"]["content"] = answer;
    resp["choices"][0]["finish_reason"] = "stop";
    
    return crow::response(resp.dump());
}

// 简化聊天接口
crow::response HttpServer::simpleChat(const crow::request &req) {
    json req_json;
    try {
        req_json = json::parse(req.body);
    } catch (const std::exception& e) {
        return crow::response(400, "Invalid JSON");
    }
    
    std::string prompt = req_json.value("prompt", "");
    if (prompt.empty()) {
        return crow::response(400, "Missing 'prompt'");
    }
    
    // 执行推理
    auto& model = LLMModel::getInstance();
    std::string answer = model.infer(prompt);
    
    json resp = {{"response", answer}};
    return crow::response(resp.dump());
}