# syntax=docker/dockerfile:1
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    python3 python3-pip \
    git \
    lcov gcovr \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN bash scripts/linux/build.sh || true

CMD ["bash"]
