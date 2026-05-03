FROM otel-cpp-ubuntu-latest as otel-source

ls /opt/opentelemetry-cpp-install

FROM base-${BASE_IMAGE}-dev AS result

ENV TZ=Europe/Moscow
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

COPY --from=otel-source /opt/opentelemetry-cpp-install /

RUN cp -r /opt/opentelemetry-cpp-install/. /usr/

RUN git clone https://github.com/oatpp/oatpp.git \
    && cd oatpp \
    && git checkout 1.3.0-latest \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make -j$(nproc) \
    && make install
