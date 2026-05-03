DOCKER_REPO := yaishenka/
TARGET := docker-example-observability
DOCKER_TAG := $(shell date +%Y%m%d)
DOCKER_LOGIN := login
DOCKER_TOKEN := token
OTEL_VERSION := v1.26.0

NPROC := $(shell nproc)

all: push

login:
	docker login -u ${DOCKER_LOGIN} -p ${DOCKER_TOKEN}

build-otel-docker:
	cd build-otel && ./build.sh -b ubuntu-latest -j${NPROC} -o ${OTEL_VERSION}

build:
	docker build \
		--build-arg DOCKER_TAG=${DOCKER_TAG} \
		-t ${DOCKER_REPO}${TARGET}:${DOCKER_TAG} \
		-t ${DOCKER_REPO}${TARGET}:latest \
		--network host \
		.
	echo Builded ${DOCKER_REPO}${TARGET}:${DOCKER_TAG}

push: build
	docker push ${DOCKER_REPO}${TARGET}:${DOCKER_TAG}
	docker push ${DOCKER_REPO}${TARGET}:latest
