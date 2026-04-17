#include <iostream>
#include <string>
#include <mutex>
#include <cstring>
#include <csignal>
#include <atomic>
#include "crow.h"
#include "nlohmann/json.hpp"

#include "rkllm.h"

using json = nlohmann::json;

// 全局 RKLLM 句柄和互斥锁
static LLMHandle g_llm_handle = nullptr;
static std::mutex g_llm_mutex;

static std::atomic<bool> shutdown_flag(false);
static crow::SimpleApp *g_app_ptr = nullptr;

void signal_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
    {
        std::cout << "\nReceived shutdown signal. Stopping server..." << std::endl;
        shutdown_flag = true;
        if (g_app_ptr)
        {
            g_app_ptr->stop(); // 停止 Crow 服务
        }
    }
}

// 回调函数上下文
struct CallbackContext
{
    std::string *output;
};

// RKLLM 回调函数（必须匹配 LLMResultCallback 签名）
int rkllm_callback(RKLLMResult *result, void *userdata, LLMCallState state)
{
    if (!result || !result->text)
        return 0;
    auto *ctx = static_cast<CallbackContext *>(userdata);
    if (ctx && ctx->output)
    {
        ctx->output->append(result->text);
    }
    // 返回 0 表示继续正常推理
    return 0;
}

// 初始化 RKLLM 模型
int init_rkllm(const char *model_path)
{
    RKLLMParam param = rkllm_createDefaultParam();
    param.model_path = model_path;
    param.max_context_len = 4096; // 上下文窗口（与模型转换时一致）
    param.max_new_tokens = 512;   // 最大输出长度
    param.top_k = 40;
    param.top_p = 0.95;
    param.temperature = 0.7;
    param.repeat_penalty = 1.1;
    param.is_async = false; // 使用同步模式

    // 如果需要指定 CPU 核心，可以通过 extend_param
    // param.extend_param.enabled_cpus_mask = CPU4 | CPU5 | CPU6 | CPU7;
    // param.extend_param.enabled_cpus_num = 4;

    int ret = rkllm_init(&g_llm_handle, &param, rkllm_callback);
    if (ret != 0)
    {
        std::cerr << "rkllm_init failed with code: " << ret << std::endl;
        return -1;
    }
    std::cout << "RKLLM initialized successfully." << std::endl;
    return 0;
}

// 执行推理
std::string run_inference(const std::string &prompt)
{
    std::lock_guard<std::mutex> lock(g_llm_mutex);
    std::string generated_text;
    CallbackContext ctx{&generated_text};

    // 构造输入
    RKLLMInput input;
    input.role = "user";
    input.enable_thinking = false; // 根据模型支持调整
    input.input_type = RKLLM_INPUT_PROMPT;
    input.prompt_input = prompt.c_str();

    // 推理参数（使用默认）
    RKLLMInferParam infer_params;
    infer_params.mode = RKLLM_INFER_GENERATE;
    infer_params.lora_params = nullptr;
    infer_params.prompt_cache_params = nullptr;
    infer_params.keep_history = 0; // 不保留历史（每次独立）

    // 执行推理
    int ret = rkllm_run(g_llm_handle, &input, &infer_params, &ctx);
    if (ret != 0)
    {
        std::cerr << "rkllm_run failed, code: " << ret << std::endl;
        return "[Error] Inference failed.";
    }
    return generated_text;
}

int main()
{
    const char *model_path = "model/Qwen2-1___5B-Instruct_W8A8_RK3588.rkllm";
    if (init_rkllm(model_path) != 0)
    {
        return -1;
    }

    crow::SimpleApp app;
    g_app_ptr = &app;

    // 注册信号处理
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // 健康检查
    CROW_ROUTE(app, "/health")
    ([]()
     { return crow::response(200, "OK"); });

    // OpenAI 兼容的聊天补全接口
    CROW_ROUTE(app, "/v1/chat/completions")
        .methods("POST"_method)([](const crow::request &req)
                                {
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

        std::string answer = run_inference(user_prompt);

        json resp;
        resp["choices"] = json::array();
        resp["choices"][0]["message"]["role"] = "assistant";
        resp["choices"][0]["message"]["content"] = answer;
        resp["choices"][0]["finish_reason"] = "stop";

        return crow::response(resp.dump()); });

    // 简化接口 /chat
    CROW_ROUTE(app, "/chat")
        .methods("POST"_method)([](const crow::request &req)
                                {
        json req_json = json::parse(req.body);
        std::string prompt = req_json.value("prompt", "");
        if (prompt.empty()) {
            return crow::response(400, "Missing 'prompt'");
        }
        std::string answer = run_inference(prompt);
        json resp = {{"response", answer}};
        return crow::response(resp.dump()); });

    std::cout << "Starting HTTP server on http://0.0.0.0:8080" << std::endl;
    app.port(8080).multithreaded().run();

    std::cout << "Server stopped. Cleaning up..." << std::endl;

    // 清理模型
    rkllm_destroy(g_llm_handle);
    std::cout << "RKLLM destroyed." << std::endl;

    return 0;
}