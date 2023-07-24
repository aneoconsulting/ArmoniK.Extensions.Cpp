#include "ComputeService.h"
#include "TaskRequest.h"
#include "interfaceDLL.h"

SDK_WORKER_NAMESPACE::ComputeService::ComputeService(/* args */)
{
}

ArmoniK::Api::Worker::ProcessStatus SDK_WORKER_NAMESPACE::ComputeService::Process(ArmoniK::Api::Worker::TaskHandler &task_handler){

    std::vector<std::byte> payload = task_handler.getPayload();
    char data_string[payload.size()];
    std::memcpy(data_string, payload.data(), payload.size());
    
    std::string_view str(data_string);

    auto deserialized = Common::TaskRequest::Deserialize(str);

    if(!deserialized.service_name.empty()){
        ArmoniK::SDK::Worker::ServiceContext service_context(deserialized);
        ArmoniK::SDK::Worker::SessionContext session_context;
        ArmoniK::SDK::Worker::ApplicationManager app;
        app.CreateService(service_context);
        app.EnterSession(session_context);
        app.Execute(service_context, session_context);
    }

    return ArmoniK::Api::Worker::ProcessStatus::OK;
}

/**
 * @brief Construct a new sdk worker namespace::servicecontext::servicecontext object
 * 
 * @param task_request 
 */
SDK_WORKER_NAMESPACE::ServiceContext::ServiceContext(const ArmoniK::SDK::Common::TaskRequest &task_request){
    application_name_ = task_request.method_name;
    service_name_ = task_request.service_name;
}

/**
 * @brief 
 * 
 * @return std::string 
 */
std::string SDK_WORKER_NAMESPACE::ServiceContext::getServiceName(){
    return service_name_;
}

/**
 * @brief 
 * 
 * @return std::string 
 */
std::string SDK_WORKER_NAMESPACE::ServiceContext::getApplicationName(){
    return application_name_;
}

/**
 * @brief 
 * 
 * @return std::string 
 */
std::string SDK_WORKER_NAMESPACE::ServiceContext::getNamespace(){
    return app_namespace_;
}

/**
 * @brief Get the Session Id object
 * 
 * @return std::string 
 */
std::string SDK_WORKER_NAMESPACE::SessionContext::getSessionId(){
    return session_id_;
}

/**
 * @brief Create a Service object
 * 
 * @param service_context 
 */
void SDK_WORKER_NAMESPACE::ApplicationManager::CreateService(ServiceContext service_context){
    void* ptr = armonik_create_service(service_context.getServiceName().c_str());
}

/**
 * @brief 
 * 
 * @param session_context 
 */
void SDK_WORKER_NAMESPACE::ApplicationManager::EnterSession(SessionContext session_context){
    void* ptr = armonik_enter_session(&session_context, session_context.getSessionId().c_str());
}

/**
 * @brief 
 * 
 * @param service_context 
 * @param session_context 
 */
void SDK_WORKER_NAMESPACE::ApplicationManager::Execute(ServiceContext service_context, SessionContext session_context){
    armonik_call(&service_context, &session_context, service_context.getApplicationName().c_str(), service_context.getApplicationName().c_str(), service_context.getApplicationName().size(), CallbackFunc);
}

/**
 * @brief 
 * 
 * @param service_context 
 * @param session_context 
 */
void SDK_WORKER_NAMESPACE::ApplicationManager::LeaveSession(ServiceContext service_context, SessionContext session_context){
    armonik_leave_session(&service_context, &session_context);
}

/**
 * @brief 
 * 
 * @param service_context 
 */
void SDK_WORKER_NAMESPACE::ApplicationManager::DestroyService(ServiceContext service_context){
    armonik_destroy_service(&service_context);
}

/**
 * @brief 
 * 
 * @param status 
 * @param output 
 * @param size 
 */
void SDK_WORKER_NAMESPACE::ApplicationManager::CallbackFunc(status_t status, const char* output, size_t size){
    if (status){
        std::cout << "ArmoniK call succedded" << output << std::endl;
    }else{
        std::cout << "ArmoniK call failed" << output << std::endl;
    }
}

