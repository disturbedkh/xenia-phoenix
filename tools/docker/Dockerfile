# Pre-baked Phoenix Linux build environment (Ubuntu 24.04 + LLVM 20 + Vulkan SDK).
# Build:  docker build -t phoenix-linux-build:24.04 -f tools/docker/Dockerfile tools/docker
# Run:    docker run --rm -v "$PWD:/src:rw" -w /src phoenix-linux-build:24.04 bash tools/docker/linux-build.sh release
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV LLVM_VERSION=20
ENV UBUNTU_BASE=noble
ENV PHOENIX_LINUX_BUILD_ENV=1

RUN apt-get update -qq && apt-get install -y -qq wget gnupg software-properties-common ca-certificates git

RUN wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc && \
    add-apt-repository -y "deb http://apt.llvm.org/${UBUNTU_BASE}/ llvm-toolchain-${UBUNTU_BASE}-${LLVM_VERSION} main" && \
    apt-get update -qq && apt-get install -y -qq \
      build-essential mesa-vulkan-drivers valgrind libc++-dev libc++abi-dev \
      libgtk-3-dev libsdl2-dev libvulkan-dev libx11-xcb-dev liblz4-dev \
      libasound2-dev libfontconfig1-dev \
      clang-${LLVM_VERSION} lld-${LLVM_VERSION} llvm-${LLVM_VERSION} ninja-build cmake \
      spirv-tools python3 pkg-config \
      xvfb libsdl2-2.0-0 libgtk-3-0 libvulkan1 libasound2t64 libfontconfig1 libfuse2

RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} 200 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} 200 && \
    update-alternatives --install /usr/bin/lld lld /usr/bin/lld-${LLVM_VERSION} 200 && \
    update-alternatives --install /usr/bin/ld.lld ld.lld /usr/bin/ld.lld-${LLVM_VERSION} 200 && \
    update-alternatives --install /usr/bin/llvm-ar llvm-ar /usr/bin/llvm-ar-${LLVM_VERSION} 200 && \
    update-alternatives --install /usr/bin/llvm-ranlib llvm-ranlib /usr/bin/llvm-ranlib-${LLVM_VERSION} 200 && \
    update-alternatives --install /usr/bin/llvm-nm llvm-nm /usr/bin/llvm-nm-${LLVM_VERSION} 200

RUN mkdir -p /root/vulkan-sdk && \
    wget -qO /tmp/vulkan-sdk.tar.xz https://sdk.lunarg.com/sdk/download/latest/linux/vulkan-sdk.tar.xz && \
    tar -xf /tmp/vulkan-sdk.tar.xz -C /root/vulkan-sdk && rm /tmp/vulkan-sdk.tar.xz

ENV VULKAN_SDK=/root/vulkan-sdk/$(ls /root/vulkan-sdk | head -n1)/x86_64
ENV PATH="${VULKAN_SDK}/bin:${PATH}"

WORKDIR /src
