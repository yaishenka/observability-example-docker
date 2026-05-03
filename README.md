# Сборка docker для лекции по observability

![Docker Pulls](https://badgen.net/docker/pulls/yaishenka/docker-example-observability) ![Docker size](https://badgen.net/docker/size/yaishenka/docker-example-observability)

[Репозиторий с лекциями по cpp](https://gitlab.com/yaishenka/cpp_course)
[Лекция 24-25 года](https://gitlab.com/yaishenka/cpp_course/-/tree/2024_2025/lectures/observability?ref_type=heads)

## Сборка

1) `git submodule update --init --recursive`
2) `make build-otel-docker` --- собирает все для opentelemetry внутри docker с ubuntu
3) `make build` ---- собирает докер с доп библиотеками (spdlog, oatpp)

## Самый простой запуск

`sudo docker run -v ./examples:/examples -p 8080:8080 -it yaishenka/docker-example-observability:latest /bin/bash`
