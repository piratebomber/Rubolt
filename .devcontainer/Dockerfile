# syntax=docker/dockerfile:1
FROM mcr.microsoft.com/vscode/devcontainers/base:ubuntu

RUN apt-get update && apt-get install -y \
    build-essential \
    python3 python3-pip \
    clang lcov gcovr \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspaces/Rubolt
COPY . .

RUN bash scripts/linux/build.sh || true
CMD ["/bin/bash"]
