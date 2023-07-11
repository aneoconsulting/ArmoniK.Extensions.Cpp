#include "../include/TaskOptions.h"

armonik::api::grpc::v1::TaskOptions get_default_task_option() {
  armonik::api::grpc::v1::TaskOptions default_task_options;
  default_task_options.mutable_max_duration()->set_seconds(300);
  default_task_options.mutable_max_duration()->set_nanos(0);
  default_task_options.set_max_retries(3);
  default_task_options.set_priority(1);
  default_task_options.set_engine_type("Unified");
  return std::move(default_task_options);
}
