#pragma once

#include <string>

#include <armonik/worker/worker_common.pb.h>
#include <armonik/worker/worker_service.grpc.pb.h>

#include <armonik/worker/Worker/ArmoniKWorker.h>
#include <armonik/worker/Worker/TaskHandler.h>

namespace SDK_WORKER_NAMESPACE
{
    
    class BaseService
    {
    private:
        /* data */

    public:
        BaseService(/* args */);
        virtual ~BaseService() = default;

        virtual void* create_service(void* service_ctx);

        virtual void* enter_session(const char* session_id);

        virtual void leave_session(void* session_ctx);

        virtual std::string call(std::string name, std::string input);

        virtual void destroy_service(void* service_ctx);

        // std::vector<std::string> SubmitwTasks(std
        // ::vector<std::byte> payloads, int max_retries, ArmoniK::SDK::Common::TaskOptions task_options);

    };
    
    
} // namespace SDK_WORKER_NAMESPACE
