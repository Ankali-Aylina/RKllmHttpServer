#include "llm_model.h"
#include <iostream>
#include <memory>

// 获取单例实例
LLMModel& LLMModel::getInstance() {
    static LLMModel instance;
    return instance;
}

// 析构函数
LLMModel::~LLMModel() {
    cleanup();
}

// 初始化模型
bool LLMModel::initialize(const std::string& model_path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        std::cout << "Model already initialized." << std::endl;
        return true;
    }
    
    RKLLMParam param = rkllm_createDefaultParam();
    param.model_path = model_path.c_str();
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
    
    int ret = rkllm_init(&m_handle, &param, rkllm_callback);
    if (ret != 0) {
        std::cerr << "rkllm_init failed with code: " << ret << std::endl;
        return false;
    }
    
    m_initialized = true;
    std::cout << "RKLLM initialized successfully from: " << model_path << std::endl;
    return true;
}

// 执行推理
std::string LLMModel::infer(const std::string& prompt) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized || !m_handle) {
        return "[Error] Model not initialized.";
    }
    
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
    int ret = rkllm_run(m_handle, &input, &infer_params, &ctx);
    if (ret != 0) {
        std::cerr << "rkllm_run failed, code: " << ret << std::endl;
        return "[Error] Inference failed.";
    }
    
    return generated_text;
}

// 清理资源
void LLMModel::cleanup() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_handle) {
        rkllm_destroy(m_handle);
        m_handle = nullptr;
        std::cout << "RKLLM destroyed." << std::endl;
    }
    
    m_initialized = false;
}

// RKLLM 回调函数（必须匹配 LLMResultCallback 签名）
int LLMModel::rkllm_callback(RKLLMResult *result, void *userdata, LLMCallState state) {
    if (!result || !result->text)
        return 0;
    
    auto *ctx = static_cast<CallbackContext *>(userdata);
    if (ctx && ctx->output) {
        ctx->output->append(result->text);
    }
    
    // 返回 0 表示继续正常推理
    return 0;
}