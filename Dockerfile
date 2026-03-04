FROM ubuntu:24.04 as builder

WORKDIR /usr/src

RUN DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC && \
  apt-get update -y && \
  apt-get -y install git make cmake clang pkg-config autoconf automake libtool

# prime_server
RUN git clone \
  --depth=1 \
  --recurse-submodules \
  --single-branch \
  --branch=master \
  https://github.com/kevinkreiser/prime_server.git prime_server

RUN apt-get install -y libzmq3-dev libczmq-dev libcurl4-openssl-dev

RUN cd prime_server && \
  mkdir build && \
  cd build && \
  cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo && \
  make -j && \
  make install && \
  ldconfig

# valhalla dependencies
RUN apt-get install -y libz-dev liblz4-dev libboost-all-dev \
  libprotobuf-dev protobuf-compiler \
  libsqlite3-dev libspatialite-dev \
  libluajit-5.1-dev libgeos++-dev

# valhalla from local source
COPY . /usr/src/valhalla

ARG version
RUN cd valhalla && \
  mkdir build && \
  cd build && \
  cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DENABLE_BENCHMARKS=OFF \
    -DENABLE_TESTS=OFF \
    -DENABLE_PYTHON_BINDINGS=OFF \
    -DENABLE_DATA_TOOLS=ON && \
  make -j$(nproc) && \
  make install && \
  ldconfig

# Build tracer against installed valhalla
RUN cd valhalla && \
  clang++ -std=c++17 -O2 -DNDEBUG tracer.cpp \
    -I/usr/local/include \
    -Ithird_party/date/include \
    -L/usr/local/lib \
    -lvalhalla -lprotobuf -lz -llz4 -lpthread \
    -o /usr/local/bin/valhalla_tracer

FROM ubuntu:24.04
COPY --from=builder /usr/local /usr/local

RUN DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get update -y && \
  apt-get install -y libprotobuf-dev libcurl4-openssl-dev libspatialite-dev \
  libluajit-5.1-dev libzmq3-dev libczmq-dev && \
  ldconfig

CMD valhalla_service /config/valhalla.json 1
