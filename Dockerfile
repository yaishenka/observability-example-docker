FROM otel-cpp-ubuntu-latest AS otel-source

RUN ls /opt/opentelemetry-cpp-install

FROM base-ubuntu-latest-dev AS result

ENV TZ=Europe/Moscow
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

COPY --from=otel-source /opt/opentelemetry-cpp-install /opentelemetry-cpp-install

RUN cp -r /opentelemetry-cpp-install/. /usr/

RUN git clone https://github.com/oatpp/oatpp.git \
    && cd oatpp \
    && git checkout 1.3.0-latest \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make -j$(nproc) \
    && make install

RUN git clone https://github.com/gabime/spdlog.git \
    && cd spdlog \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make -j$(nproc) \
    && make install

WORKDIR /

RUN apt-get update && apt-get install -y --no-install-recommends vim
