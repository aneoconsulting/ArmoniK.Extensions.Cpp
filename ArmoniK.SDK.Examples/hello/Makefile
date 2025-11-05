WORKING_DIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/../../)

.ONESHELL:
CLIENT_IMAGE_TAG?= armonik-cpp-hello-client:0.1.0-sdk
WORKER_IMAGE_TAG?= armonik-cpp-hello-worker:0.1.0-sdk
API_VERSION?= 3.27.0
CLIENT_VERSION?= 0.1.0-local
DYN_WORKER_NAME?= armonik-sdk-cpp-dynamicworker
DYN_WORKER_TAG?= 0.1.0-local-alpine
WORKER_SDK_VERSION?= 0.1.0-local
ARMONIK_SHARE_DATA_DIR?= $(realpath ${WORKING_DIR}/../ArmoniK/infrastructure/quick-deploy/localhost/data/)

.PHONY: help
help: 	## Show help
	@echo "Usage: make <target>"
	@echo ""
	@echo "Targets:"
	@fgrep "##" Makefile | fgrep -v fgrep

all: build_worker build_client

echo:
	@echo ${ARMONIK_SHARE_DATA_DIR}

.PHONY: build_client
build_client:
	@docker build -t ${CLIENT_IMAGE_TAG} \
		-f "${WORKING_DIR}/ArmoniK.SDK.Examples/hello/client/Dockerfile" \
		--build-arg="API_VERSION=${API_VERSION}" \
		--build-arg="CLIENT_VERSION=${CLIENT_VERSION}" \
		${WORKING_DIR}

.PHONY: build_dynworker
build_dynamic_worker:
	"${WORKING_DIR}"/ArmoniK.SDK.DynamicWorker/build.sh "${WORKING_DIR}/ArmoniK.SDK.DynamicWorker/Dockerfile" "${DYN_WORKER_NAME}:${DYN_WORKER_TAG}" "${API_VERSION}" "${WORKER_SDK_VERSION}"

.PHONY: build_worker
build_worker: build_dynamic_worker
	@docker build -t ${WORKER_IMAGE_TAG} \
		-f "${WORKING_DIR}/ArmoniK.SDK.Examples/hello/worker/Dockerfile" \
		--build-arg="DynamicWorkerImage=${DYN_WORKER_NAME}:${DYN_WORKER_TAG}" \
		--build-arg="API_VERSION=${API_VERSION}" \
		--build-arg="WorkerLibVersion=${WORKER_SDK_VERSION}" \
		${WORKING_DIR}
	@echo "Copying shared object to ${ARMONIK_SHARE_DATA_DIR}"
	@echo "You should manually copy them if this is not correct for your deployment"
	@docker run --rm -v "${ARMONIK_SHARE_DATA_DIR}:/host" --entrypoint sh \
		"${WORKER_IMAGE_TAG}" -c "cp /app/install/lib*/*.so /app/install/lib*/*.*so /host/"