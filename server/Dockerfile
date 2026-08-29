FROM rust:1.97 AS builder
WORKDIR /usr/src/server
COPY . .
RUN cargo install --path .

FROM scratch
COPY --from=builder /usr/local/cargo/bin/deathmarkers-server /bin/deathmarkers-server
CMD ["deathmarkers-server"]
