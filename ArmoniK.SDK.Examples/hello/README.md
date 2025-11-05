# Hello World Example using ArmoniK's C++ SDK

This project demonstrates a "Hello World" example using the ArmoniK C++ SDK. The client sends a task with the payload "Hello," and the worker takes this task, appending " World!" to the input.

## Overview

1. **Client**: Sends a task with the payload **"Hello,"**.
2. **Worker**: Receives the task and appends **" World!"** to the input.

## Makefile

The provided `Makefile` includes two recipes:
- **build_client**: This recipe builds the client application.
- **build_worker**: This recipe builds the worker application, which requires the dynamic worker provided by the SDK, so it will attempt to build it before proceeding.

## Running the Example

To successfully run this example, take the following steps:

1. Add a `cppdynamic` partition to your infrastructure.

    ```diff
    +cppdynamic = {
    +  # number of replicas for each deployment of compute plane
    +  replicas = 0
    +  # ArmoniK polling agent
    +  polling_agent = {...
    +  }
    +  # ArmoniK workers
    +  worker = [
    +    {
    +      image = "armonik-sdk-cpp-dynamicworker"
    +      tag = "0.0.0-local" 
    +      limits = {...}
    +      requests = {...}
    +    }
    +  ]
    +  hpa = {...
    +  }
    +}

2. Redeploy ArmoniK to include the new partition.
3. To execute the client, run the following command:

```bash
docker run --rm -e GrpcClient__Endpoint=http://192.168.6.41:5001 -e WorkerLib_Version=0.1.0-local -e PartitionId=cppdynamic armonik-cpp-hello-client:0.1.0-sdk
```

## Notes:
- Ensure that you replace http://192.168.6.41:5001 with the correct endpoint for your control plane.
- The tags used for all the containers are defined in the Makefile, you can modify them by defining suitable environment variables