#include "ComputeService.h"
#include "TaskRequest.h"
#include "ArmoniKSDKInterface.h"

SDK_WORKER_NAMESPACE::ComputeService::ComputeService(/* args */)
{
}

ArmoniK::Api::Worker::ProcessStatus SDK_WORKER_NAMESPACE::ComputeService::Process(ArmoniK::Api::Worker::TaskHandler &task_handler){

    std::string payload = task_handler.getPayload();

    auto deserialized = Common::TaskRequest::Deserialize(payload);

    if(!deserialized.service_name.empty()){
        ArmoniK::SDK::Worker::ServiceContext service_context(deserialized);
        ArmoniK::SDK::Worker::SessionContext session_context;
        ArmoniK::SDK::Worker::ApplicationManager app;
        app.CreateService(service_context);
        app.EnterSession(session_context);
        app.Execute(service_context, session_context);
    }

    return ArmoniK::Api::Worker::ProcessStatus::PROCESS_OK;
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
SDK_WORKER_NAMESPACE::ServiceContext* SDK_WORKER_NAMESPACE::ApplicationManager::CreateService(ServiceContext service_context){
    service_context_ = (SDK_WORKER_NAMESPACE::ServiceContext*) armonik_create_service(service_context.getServiceName().c_str());

    return service_context_;
}

/**
 * @brief 
 * 
 * @param session_context 
 */
SDK_WORKER_NAMESPACE::SessionContext* SDK_WORKER_NAMESPACE::ApplicationManager::EnterSession(SessionContext session_context){
    session_context_ = (SDK_WORKER_NAMESPACE::SessionContext*) armonik_enter_session(&session_context, session_context.getSessionId().c_str());

    return session_context_;
}

/**
 * @brief 
 * 
 * @param service_context 
 * @param session_context 
 */
void SDK_WORKER_NAMESPACE::ApplicationManager::Execute(ServiceContext service_context, SessionContext session_context){
    armonik_call(&service_context, &session_context, service_context.getApplicationName().c_str(), service_context.getApplicationName().c_str(), service_context.getApplicationName().size(), SDK_WORKER_NAMESPACE::ApplicationManager::CallbackFunc);
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
    // delete service_context_;
    // delete session_context_;
}

