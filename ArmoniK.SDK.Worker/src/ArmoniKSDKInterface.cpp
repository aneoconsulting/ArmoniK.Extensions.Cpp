#include <string>
#include "BaseService.h"
#include "ArmoniKSDKInterface.h"

/**
 * @brief 
 * 
 * @param name 
 * @return void* 
 */
#ifdef __linux__
__attribute__((weak))
#endif
void* armonik_create_service(const char*) {
    return nullptr;
}

/**
 * @brief 
 * 
 * @param service_context 
 */
#ifdef __linux__
__attribute__((weak))
#endif
void armonik_destroy_service(void* p) {
    if (p) {
        delete static_cast<BaseService*>(p);
    }
}

/**
 * @brief 
 * 
 * @param service_context 
 * @param session_id 
 * @return void* 
 */
#ifdef __linux__
__attribute__((weak))
#endif
void* armonik_enter_session(void* service_context, const char* session_id) {
    return static_cast<BaseService*>(service_context)->enter_session(session_id);
}

/**
 * @brief 
 * 
 * @param service_context 
 * @param session_context 
 */
#ifdef __linux__
__attribute__((weak))
#endif
void armonik_leave_session(void* service_context, void* session_context) {
    static_cast<BaseService*>(service_context)->leave_session(session_context);
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
#ifdef __linux__
__attribute__((weak))
#endif
void armonik_call(
    void* service_context,
    void* session_context,
    const char* function_name,
    const char* input,
    size_t input_size,
    void (*callback)(armonik_status_t status, const char* output, size_t output_size)){
    try {
        auto output = static_cast<BaseService*>(service_context)->call(std::string(function_name, strlen(function_name)), std::string(input, input_size));
        callback(0, output.data(), output.size());
    } catch (const std::exception& e) {
        auto msg = e.what();
        callback(1, msg, strlen(msg));
    }
}
