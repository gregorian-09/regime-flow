FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        libprotobuf-dev \
        protobuf-compiler \
        libssl-dev \
        libcurl4-openssl-dev \
        libboost-system-dev \
        libgtest-dev \
        python3 \
        python3-pip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN cmake -S . -B build -DBUILD_TESTS=ON && \
    cmake --build build -j

CMD ["ctest", "--test-dir", "build", "--output-on-failure"]
