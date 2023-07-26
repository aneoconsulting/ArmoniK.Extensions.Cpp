#include "BaseService.h"


void* BaseService::create_service(void* service_ctx){
    return nullptr;
}

void* BaseService::enter_session(const char* session_id){
    return nullptr;
}

void BaseService::leave_session(void* session_ctx){

}

void BaseService::destroy_service(void* service_ctx){

}


