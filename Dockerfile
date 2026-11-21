FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    git cmake g++ qt6-base-dev qt6-declarative-dev \
    libxi6 libxrender1 libxcomposite1 libfontconfig1 \
    && rm -rf /var/lib/apt/lists/*

# Clone and build AllegroScore
WORKDIR /app
RUN git clone https://github.com/durkimat/AllegroScore.git ./

# Build the project
RUN cmake -B build .
RUN cmake --build build -j$(nproc)

# Runtime stage
FROM ubuntu:22.04

# Install runtime Qt dependencies only
RUN apt-get update && apt-get install -y \
    qt6-base-run qt6-qtbase \
    libxi6 libxrender1 libxcomposite1 libfontconfig1 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/AllegroScore /usr/local/bin/AllegroScore

# Set display env for offscreen mode by default
ENV QT_QPA_PLATFORM=offscreen

ENTRYPOINT ["/usr/local/bin/AllegroScore"]