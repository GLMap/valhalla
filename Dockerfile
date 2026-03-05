FROM alpine:latest AS builder

WORKDIR /usr/src

RUN apk add --no-cache \
    build-base \
    cmake \
    clang \
    git \
    autoconf \
    automake \
    libtool \
    pkgconf \
    boost-dev \
    protobuf \
    protobuf-dev \
    sqlite-dev \
    libspatialite-dev \
    luajit-dev \
    geos-dev \
    lz4-dev \
    zlib-dev \
    zeromq-dev \
    czmq-dev \
    curl-dev

# prime_server
RUN git clone \
  --depth=1 \
  --recurse-submodules \
  --single-branch \
  --branch=master \
  https://github.com/kevinkreiser/prime_server.git prime_server

RUN cd prime_server && \
  mkdir build && \
  cd build && \
  cmake .. -DCMAKE_BUILD_TYPE=Release && \
  make -j$(nproc) && \
  make install

# valhalla from local source (includes tracer target in CMakeLists.txt)
COPY . /usr/src/valhalla

ARG version
RUN cd valhalla && \
  mkdir build && \
  cd build && \
  cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
    -DENABLE_WERROR=OFF -DENABLE_SINGLE_FILES_WERROR=OFF \
    -DProtobuf_PROTOC_EXECUTABLE=/usr/bin/protoc \
    -DENABLE_BENCHMARKS=OFF \
    -DENABLE_TESTS=OFF \
    -DENABLE_PYTHON_BINDINGS=OFF \
    -DENABLE_DATA_TOOLS=ON && \
  make -j$(nproc) && \
  make install

# Intermediate stage: strip binaries for production
FROM builder AS stripped
RUN strip /usr/local/bin/valhalla_* /usr/local/lib/libvalhalla.so /usr/local/lib/libprime_server.so* 2>/dev/null; true

# --- Production image (stripped) ---
FROM alpine:latest AS production
COPY --from=stripped /usr/local/bin/valhalla_service /usr/local/bin/
COPY --from=stripped /usr/local/bin/valhalla_tracer /usr/local/bin/
COPY --from=stripped /usr/local/bin/valhalla_build_tiles /usr/local/bin/
COPY --from=stripped /usr/local/bin/valhalla_build_config /usr/local/bin/
COPY --from=stripped /usr/local/bin/valhalla_build_extract /usr/local/bin/
COPY --from=stripped /usr/local/bin/valhalla_build_admins /usr/local/bin/
COPY --from=stripped /usr/local/lib/libprime_server.so* /usr/local/lib/

ENV LD_LIBRARY_PATH=/usr/local/lib

RUN apk add --no-cache \
    libprotobuf-lite \
    libcurl \
    libspatialite \
    luajit \
    zeromq \
    czmq \
    libstdc++ \
    lz4-libs \
    boost1.84-filesystem \
    boost1.84-program_options \
    boost1.84-system

CMD ["valhalla_service", "/config/valhalla.json", "1"]

# --- Debug image (unstripped + jemalloc profiling) ---
FROM alpine:latest AS debug
COPY --from=builder /usr/local/bin/valhalla_service /usr/local/bin/
COPY --from=builder /usr/local/bin/valhalla_tracer /usr/local/bin/
COPY --from=builder /usr/local/bin/valhalla_build_tiles /usr/local/bin/
COPY --from=builder /usr/local/bin/valhalla_build_config /usr/local/bin/
COPY --from=builder /usr/local/bin/valhalla_build_extract /usr/local/bin/
COPY --from=builder /usr/local/bin/valhalla_build_admins /usr/local/bin/
COPY --from=builder /usr/local/lib/libprime_server.so* /usr/local/lib/

ENV LD_LIBRARY_PATH=/usr/local/lib

RUN apk add --no-cache \
    libprotobuf-lite \
    libcurl \
    libspatialite \
    luajit \
    zeromq \
    czmq \
    libstdc++ \
    lz4-libs \
    boost1.84-filesystem \
    boost1.84-program_options \
    boost1.84-system \
    build-base \
    graphviz \
    perl && \
    wget -qO- https://github.com/jemalloc/jemalloc/releases/download/5.3.0/jemalloc-5.3.0.tar.bz2 | tar xj && \
    cd jemalloc-5.3.0 && \
    ./configure --enable-prof && \
    make -j$(nproc) && \
    make install && \
    cd .. && rm -rf jemalloc-5.3.0 && \
    apk del build-base

# Usage: run with these env vars to get heap profiles:
#   LD_PRELOAD=/usr/local/lib/libjemalloc.so.2
#   MALLOC_CONF=prof:true,prof_prefix:/tmp/jeprof,lg_prof_interval:30,lg_prof_sample:17
# Analyze: jeprof --svg /usr/local/bin/valhalla_build_tiles /tmp/jeprof.*.heap > profile.svg

CMD ["valhalla_service", "/config/valhalla.json", "1"]
