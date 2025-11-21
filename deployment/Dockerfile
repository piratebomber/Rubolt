# Multi-stage Dockerfile for Rubolt applications
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    g++ \
    make \
    cmake \
    git \
    curl \
    wget \
    pkg-config \
    libcurl4-openssl-dev \
    libjson-c-dev \
    libssl-dev \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY . .

# Build Rubolt
RUN chmod +x build_all.sh && ./build_all.sh

# Install Rubolt
RUN bash Installer/install.sh --mode build --prefix /usr/local --add-to-path

# Production stage
FROM ubuntu:22.04 AS runtime

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libcurl4 \
    libjson-c5 \
    libssl3 \
    zlib1g \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create rubolt user
RUN useradd -r -s /bin/false rubolt

# Copy Rubolt installation from builder
COPY --from=builder /usr/local/bin/rbcli /usr/local/bin/
COPY --from=builder /usr/local/bin/rubolt /usr/local/bin/
COPY --from=builder /usr/local/lib/rubolt /usr/local/lib/rubolt
COPY --from=builder /usr/local/share/rubolt /usr/local/share/rubolt

# Set up environment
ENV PATH="/usr/local/bin:${PATH}"
ENV RUBOLT_PATH="/usr/local/lib/rubolt"

# Create app directory
WORKDIR /app
RUN chown rubolt:rubolt /app

# Switch to rubolt user
USER rubolt

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD rbcli version || exit 1

# Default command
CMD ["rbcli", "repl"]

# Development stage
FROM builder AS development

# Install development tools
RUN apt-get update && apt-get install -y \
    gdb \
    valgrind \
    strace \
    vim \
    nano \
    htop \
    && rm -rf /var/lib/apt/lists/*

# Install Node.js for registry server
RUN curl -fsSL https://deb.nodesource.com/setup_18.x | bash - \
    && apt-get install -y nodejs

# Set up development environment
WORKDIR /workspace
COPY . .

# Install development dependencies
RUN npm install -g nodemon pm2

# Expose common ports
EXPOSE 3000 8080 9229

# Development command
CMD ["bash"]

# Registry server stage
FROM node:18-alpine AS registry

WORKDIR /app

# Copy package files
COPY registry/package*.json ./

# Install dependencies
RUN npm ci --only=production

# Copy registry source
COPY registry/ .

# Create packages directory
RUN mkdir -p packages

# Expose port
EXPOSE 3000

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=10s --retries=3 \
    CMD wget --no-verbose --tries=1 --spider http://localhost:3000/health || exit 1

# Start registry server
CMD ["node", "server.js"]

# Web IDE stage
FROM node:18-alpine AS web-ide

WORKDIR /app

# Install dependencies
RUN npm install -g \
    @theia/cli \
    @theia/core \
    @theia/filesystem \
    @theia/editor \
    @theia/monaco \
    @theia/terminal

# Copy Theia configuration
COPY deployment/theia/ .

# Build Theia application
RUN theia build

# Expose port
EXPOSE 3000

# Start Theia
CMD ["node", "src-gen/backend/main.js", "--hostname=0.0.0.0"]

# Monitoring stage
FROM prom/prometheus:latest AS monitoring

# Copy Prometheus configuration
COPY monitoring/prometheus.yml /etc/prometheus/

# Expose port
EXPOSE 9090

# Production deployment stage
FROM runtime AS production

# Copy application code
COPY --chown=rubolt:rubolt . /app/

# Install application dependencies
USER root
RUN cd /app && rbcli install
USER rubolt

# Build application
RUN cd /app && rbcli build --release

# Expose application port
EXPOSE 8080

# Start application
CMD ["rbcli", "run"]