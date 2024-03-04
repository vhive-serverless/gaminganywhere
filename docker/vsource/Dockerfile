# Server DockerFile
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/users/yankai14/gaminganywhere/deps.posix/lib:/usr/local/lib
ENV GADEPS=/users/yankai14/gaminganywhere/deps.posix
ENV PKG_CONFIG_PATH=$GADEPS/lib/pkgconfig:/opt/local/lib/pkgconfig:/usr/lib/i386-linux-gnu/pkgconfig/:/usr/lib/pkgconfig
ENV PATH=$GADEPS/bin:$PATH

# Install dependencies using sudo apt-get
RUN apt-get update &&  apt-get install -y patch make cmake g++ pkg-config \
  	libx11-dev libxext-dev libxtst-dev libfreetype6-dev \
  	libgl1-mesa-dev libglu1-mesa-dev \
  	libpulse-dev libasound2-dev lib32z1 git lsb-release curl gpg redis bzip2

WORKDIR /users/yankai14/gaminganywhere

# Install redis cpp sdk
RUN mkdir temp \
    && cd temp \
    && git clone https://github.com/redis/hiredis.git \
    && cd hiredis \
    && make \
    && make install \
    && cd .. \
    && git clone https://github.com/sewenew/redis-plus-plus.git \
    && cd redis-plus-plus \
    && mkdir build \
    && cd build \
    && cmake -DREDIS_PLUS_PLUS_CXX_STANDARD=11 .. \
    && make \
    && make install \
    && cd ..

COPY . /users/yankai14/gaminganywhere

# COPY /lib/x86_64-linux-gnu/libc.so.6 /lib/x86_64-linux-gnu

RUN mkdir -p /users/yankai14/gaminganywhere/deps.posix

# Compilation
RUN ./setup.sh && chmod +x run-server.sh

# Run executable
EXPOSE 50051

ENTRYPOINT ["/bin/bash", "-c", "./run-server.sh"]