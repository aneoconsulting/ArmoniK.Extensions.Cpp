import logging
import os

import grpc
from armonik.worker import ArmoniKWorker, TaskHandler, ClefLogger
from armonik.common import Output
from typing import List, Union
from dataclasses import dataclass, field

ClefLogger.setup_logging(logging.INFO)

@dataclass
class Request:
    method_name: str = ""
    arguments: bytes = bytes()
    data_dependencies: List[str] = field(default_factory=list)

    @staticmethod
    def deserialize(payload: bytearray) -> "Request":
        position = 0
        size_width = 8
        fieldSize = int(payload[position:position+size_width], 16)
        position+=size_width
        method_name : str = str(payload[position:position+fieldSize])
        position+=fieldSize
        fieldSize = int(payload[position:position+size_width], 16)
        position+=size_width
        arguments : bytes = payload[position:position+fieldSize]
        position+=fieldSize
        dd : List[str] = []
        while position < len(payload):
            fieldSize = int(payload[position:position+size_width], 16)
            position+=size_width
            dd.append(str(payload[position:position+fieldSize]))
            position+=fieldSize
        return Request(method_name=method_name, arguments=arguments, data_dependencies=dd)



# Task processing
def processor(task_handler: TaskHandler) -> Output:
    logger = ClefLogger.getLogger("ArmoniKWorker")
    
    logger.info(f"Received payload of size : {len(task_handler.payload)}")
    req = Request.deserialize(task_handler.payload)
    logger.info(f"Deserialized payload : {req}")

    task_handler.send_result(task_handler.expected_results[0], bytes(task_handler.payload))
    
    return Output()


def main():
    # Create Seq compatible logger
    logger = ClefLogger.getLogger("ArmoniKWorker")
    # Define agent-worker communication endpoints
    worker_scheme = "unix://" if os.getenv("ComputePlane__WorkerChannel__SocketType", "unixdomainsocket") == "unixdomainsocket" else "http://"
    agent_scheme = "unix://" if os.getenv("ComputePlane__AgentChannel__SocketType", "unixdomainsocket") == "unixdomainsocket" else "http://"
    worker_endpoint = worker_scheme+os.getenv("ComputePlane__WorkerChannel__Address", "/cache/armonik_worker.sock")
    agent_endpoint = agent_scheme+os.getenv("ComputePlane__AgentChannel__Address", "/cache/armonik_agent.sock")

    # Start worker
    logger.info("Worker Started")
    with grpc.insecure_channel(agent_endpoint) as agent_channel:
        worker = ArmoniKWorker(agent_channel, processor, logger=logger)
        logger.info("Worker Connected")
        worker.start(worker_endpoint)


if __name__ == "__main__":
    main()
