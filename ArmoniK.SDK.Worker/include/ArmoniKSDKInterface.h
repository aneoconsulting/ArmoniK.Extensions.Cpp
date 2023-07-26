#include <stddef.h>

/**
 * Low level interface
 */

extern "C" {
typedef int armonik_status_t;

/**
 * @brief 
 * 
 * @param name 
 * @return void* 
 */
void* armonik_create_service(const char* name);

/**
 * @brief 
 * 
 * @param service_context 
 */
void armonik_destroy_service(void* service_context);

/**
 * @brief 
 * 
 * @param service_context 
 * @param session_id 
 * @return void* 
 */
void* armonik_enter_session(void* service_context, const char* session_id);

/**
 * @brief 
 * 
 * @param service_context 
 * @param session_context 
 */
void armonik_leave_session(void* service_context, void* session_context);

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
void armonik_call(
    void* service_context,
    void* session_context,
    const char* function_name,
    const char* input,
    size_t input_size, void (*callback)(armonik_status_t status, const char *output, size_t output_size));
}
