FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /usr/src

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    build-essential \
    cmake \
    clang \
    git \
    autoconf \
    automake \
    libtool \
    pkg-config \
    libboost-all-dev \
    libfmt-dev \
    protobuf-compiler \
    libprotobuf-dev \
    libsqlite3-dev \
    libspatialite-dev \
    libluajit-5.1-dev \
    libgeos-dev \
    liblz4-dev \
    zlib1g-dev \
    libzmq3-dev \
    libczmq-dev \
    libcurl4-openssl-dev \
    libssl-dev \
  && rm -rf /var/lib/apt/lists/*

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
FROM ubuntu:24.04 AS production

ENV DEBIAN_FRONTEND=noninteractive

COPY --from=stripped /usr/local/bin/valhalla_service /usr/local/bin/
COPY --from=stripped /usr/local/bin/valhalla_tracer /usr/local/bin/
COPY --from=stripped /usr/local/bin/valhalla_build_tiles /usr/local/bin/
COPY --from=stripped /usr/local/bin/valhalla_build_config /usr/local/bin/
COPY --from=stripped /usr/local/bin/valhalla_build_extract /usr/local/bin/
COPY --from=stripped /usr/local/bin/valhalla_build_admins /usr/local/bin/
COPY --from=stripped /usr/local/bin/valhalla_assign_speeds /usr/local/bin/
COPY --from=stripped /usr/local/bin/valhalla_add_predicted_traffic /usr/local/bin/
COPY --from=stripped /usr/local/lib/libprime_server.so* /usr/local/lib/

RUN apt-get update && apt-get install -y --no-install-recommends \
    libprotobuf-lite32t64 \
    libcurl4t64 \
    libspatialite8 \
    libluajit-5.1-2 \
    libzmq5 \
    libczmq4t64 \
    libstdc++6 \
    liblz4-1 \
    libboost-filesystem1.83.0 \
    libboost-program-options1.83.0 \
    libboost-system1.83.0 \
  && rm -rf /var/lib/apt/lists/*

RUN ldconfig

CMD ["valhalla_service", "/config/valhalla.json", "1"]

# --- Debug image (unstripped + jemalloc profiling) ---
FROM ubuntu:24.04 AS debug

ENV DEBIAN_FRONTEND=noninteractive

COPY --from=builder /usr/local/bin/valhalla_service /usr/local/bin/
COPY --from=builder /usr/local/bin/valhalla_tracer /usr/local/bin/
COPY --from=builder /usr/local/bin/valhalla_build_tiles /usr/local/bin/
COPY --from=builder /usr/local/bin/valhalla_build_config /usr/local/bin/
COPY --from=builder /usr/local/bin/valhalla_build_extract /usr/local/bin/
COPY --from=builder /usr/local/bin/valhalla_build_admins /usr/local/bin/
COPY --from=builder /usr/local/bin/valhalla_assign_speeds /usr/local/bin/
COPY --from=builder /usr/local/bin/valhalla_add_predicted_traffic /usr/local/bin/
COPY --from=builder /usr/local/lib/libprime_server.so* /usr/local/lib/

RUN apt-get update && apt-get install -y --no-install-recommends \
    libprotobuf-lite32t64 \
    libcurl4t64 \
    libspatialite8 \
    libluajit-5.1-2 \
    libzmq5 \
    libczmq4t64 \
    libstdc++6 \
    liblz4-1 \
    libboost-filesystem1.83.0 \
    libboost-program-options1.83.0 \
    libboost-system1.83.0 \
    ca-certificates \
    build-essential \
    bzip2 \
    graphviz \
    perl \
    wget \
  && wget -qO- https://github.com/jemalloc/jemalloc/releases/download/5.3.0/jemalloc-5.3.0.tar.bz2 | tar xj \
  && cd jemalloc-5.3.0 \
  && ./configure --enable-prof \
  && make -j$(nproc) \
  && make install \
  && cd .. && rm -rf jemalloc-5.3.0 \
  && apt-get purge -y build-essential bzip2 wget \
  && apt-get autoremove -y \
  && rm -rf /var/lib/apt/lists/*

RUN ldconfig

# Usage: run with these env vars to get heap profiles:
#   LD_PRELOAD=/usr/local/lib/libjemalloc.so.2
#   MALLOC_CONF=prof:true,prof_prefix:/tmp/jeprof,lg_prof_interval:30,lg_prof_sample:17
# Analyze: jeprof --svg /usr/local/bin/valhalla_build_tiles /tmp/jeprof.*.heap > profile.svg

CMD ["valhalla_service", "/config/valhalla.json", "1"]
