#include "BaseService.h"


void* SDK_WORKER_NAMESPACE::BaseService::create_service(void* service_ctx){
    return nullptr;
}

void* SDK_WORKER_NAMESPACE::BaseService::enter_session(const char* session_id){
    return nullptr;
}

void SDK_WORKER_NAMESPACE::BaseService::leave_session(void* session_ctx){

}

void SDK_WORKER_NAMESPACE::BaseService::destroy_service(void* service_ctx){

}


