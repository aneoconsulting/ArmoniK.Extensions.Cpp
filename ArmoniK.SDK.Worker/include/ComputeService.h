#include <grpc++/grpc++.h>
#include <armonik/worker/Worker/TaskHandler.h>
#include <armonik/worker/Worker/ProcessStatus.h>
#include "TaskOptions.h"
#include "TaskRequest.h"
#include "interfaceDLL.h"


namespace ArmoniK::SDK::Common {
class Properties;
class TaskRequest;
class TaskOptions;
} // namespace ArmoniK::SDK::Common

ArmoniK::Api::Worker::ProcessStatus Process(Api::Worker::TaskHandler &task_handler);

namespace SDK_WORKER_NAMESPACE{
class ComputeService
{
private:
    /* data */
public:
    /**
     * @brief 
     * 
     */
    ComputeService(/* args */);

    static ArmoniK::Api::Worker::ProcessStatus Process(Api::Worker::TaskHandler task_handler);
};

class ServiceContext{
private:
    std::string application_name_;
    std::string service_name_;
    std::string app_namespace_;
public:
    /**
     * @brief Construct a new Service Context object
     * 
     */
    ServiceContext(const ArmoniK::SDK::Common::TaskRequest &task_request);

    /**
     * @brief Get the Service Name object
     * 
     * @return std::string 
     */
    std::string getServiceName();

    /**
     * @brief Get the Application Name object
     * 
     * @return std::string 
     */
    std::string getApplicationName();

    /**
     * @brief Get the Namespace object
     * 
     * @return std::string 
     */
    std::string getNamespace();
};

class SessionContext{
private:
    std::string session_id_;

public:
    /**
     * @brief Construct a new Session Context object
     * 
     */
    SessionContext();

    /**
     * @brief Get the Session Id object
     * 
     * @return std::string 
     */
    std::string getSessionId();

};

class ApplicationManager
{
private:


public:
    /**
     * @brief Construct a new Application Manager object
     * 
     */
    ApplicationManager();

    /**
     * @brief Create a Service object
     * 
     * @param service_context 
     */
    void CreateService(ServiceContext service_context);

    /**
     * @brief 
     * 
     * @param session_context 
     */
    void EnterSession(SessionContext session_context);

    /**
     * @brief 
     * 
     * @param service_context 
     * @param session_context 
     */
    void Execute(ServiceContext service_context, SessionContext session_context);

    /**
     * @brief 
     * 
     * @param service_context 
     * @param session_context 
     */
    void LeaveSession(ServiceContext service_context, SessionContext session_context);

    /**
     * @brief 
     * 
     * @param service_context 
     */
    void DestroyService(ServiceContext service_context);

    /**
     * @brief 
     * 
     * @param status 
     * @param output 
     * @param size 
     */
    static void CallbackFunc(status_t status, const char* output, size_t size);
};

} // namespace SDK_WORKER_NAMESPACE