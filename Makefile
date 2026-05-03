DOCKER_REPO := yaishenka/
TARGET := docker-example-observability
DOCKER_TAG := $(shell date +%Y%m%d)
DOCKER_LOGIN := login
DOCKER_TOKEN := token
OTEL_VERSION := v1.26.0

all: push

login:
	docker login -u ${DOCKER_LOGIN} -p ${DOCKER_TOKEN}

build-otel-docker:
	rm opentelemetry-cpp/docker/Dockerfile
	cp build-otel-patch/Dockerfile opentelemetry-cpp/docker/Dockerfile
	opentelemetry-cpp/docker/build.sh build.sh -b ubuntu-latest -j($nproc) -o ${OTEL_VERSION}

build:
	docker build \
		--build-arg DOCKER_TAG=${DOCKER_TAG} \
		-t ${DOCKER_REPO}${TARGET}:${DOCKER_TAG} \
		-t ${DOCKER_REPO}${TARGET}:latest \
		--network host \
		.