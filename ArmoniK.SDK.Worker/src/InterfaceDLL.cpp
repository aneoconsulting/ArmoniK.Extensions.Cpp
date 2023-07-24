#include <string>
#include "BaseService.h"
#include "interfaceDLL.h"

/**
 * @brief 
 * 
 * @param name 
 * @return void* 
 */
__attribute__((weak))
void* armonik_create_service(const char*) {
    return nullptr;
}

/**
 * @brief 
 * 
 * @param service_context 
 */
__attribute__((weak))
void armonik_destroy_service(void* p) {
    if (p) {
        delete static_cast<SDK_WORKER_NAMESPACE::BaseService*>(p);
    }
}

/**
 * @brief 
 * 
 * @param service_context 
 * @param session_id 
 * @return void* 
 */
__attribute__((weak))
void* armonik_enter_session(void* service_context, const char* session_id) {
    return static_cast<SDK_WORKER_NAMESPACE::BaseService*>(service_context)->enter_session(session_id);
}

/**
 * @brief 
 * 
 * @param service_context 
 * @param session_context 
 */
__attribute__((weak))
void armonik_leave_session(void* service_context, void* session_context) {
    static_cast<SDK_WORKER_NAMESPACE::BaseService*>(service_context)->leave_session(session_context);
}

/**
 * @brief 
 * 
 * @param service_context 
 * @param session_context 
 * @param function_name 
 * @param input 
 * @param input_size 
 * @param callback 
 */
__attribute__((weak))
void armonik_call(
    void* service_context,
    void* session_context,
    const char* function_name,
    const char* input,
    size_t input_size,
    void (*callback)(status_t status, const char* output, size_t output_size)){
    try {
        auto output = static_cast<SDK_WORKER_NAMESPACE::BaseService*>(service_context)->call(std::string(function_name, strlen(function_name)), std::string(input, input_size));
        callback(0, output.data(), output.size());
    } catch (const std::exception& e) {
        auto msg = e.what();
        callback(1, msg, strlen(msg));
    }
}