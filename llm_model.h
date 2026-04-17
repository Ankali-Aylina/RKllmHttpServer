#ifndef LLM_MODEL_H
#define LLM_MODEL_H

#include <string>
#include <mutex>
#include "rkllm.h"

// 模型管理类
class LLMModel {
public:
    // 获取单例实例
    static LLMModel& getInstance();
    
    // 初始化模型
    bool initialize(const std::string& model_path);
    
    // 执行推理
    std::string infer(const std::string& prompt);
    
    // 清理资源
    void cleanup();
    
    // 检查是否已初始化
    bool isInitialized() const { return m_handle != nullptr; }
    
    // 禁止拷贝和赋值
    LLMModel(const LLMModel&) = delete;
    LLMModel& operator=(const LLMModel&) = delete;
    
private:
    LLMModel() = default;
    ~LLMModel();
    
    // RKLLM回调函数
    static int rkllm_callback(RKLLMResult *result, void *userdata, LLMCallState state);
    
    // 回调函数上下文
    struct CallbackContext {
        std::string* output;
    };
    
    LLMHandle m_handle = nullptr;
    std::mutex m_mutex;
    bool m_initialized = false;
};

#endif // LLM_MODEL_H