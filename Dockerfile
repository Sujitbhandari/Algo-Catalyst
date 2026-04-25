FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build -j"$(nproc)"

FROM ubuntu:22.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /src/build/AlgoCatalyst ./AlgoCatalyst
COPY --from=builder /src/data ./data
COPY --from=builder /src/scripts ./scripts
COPY --from=builder /src/requirements.txt .
COPY --from=builder /src/config.json .

RUN pip3 install --no-cache-dir -r requirements.txt

ENTRYPOINT ["./AlgoCatalyst"]
CMD ["--help"]
