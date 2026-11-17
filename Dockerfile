FROM ubuntu:24.04 AS build

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential g++-14 cmake ninja-build git curl zip unzip tar \
        pkg-config nasm python3 autoconf automake libtool linux-libc-dev \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

ENV CC=gcc-14 CXX=g++-14

ENV VCPKG_ROOT=/opt/vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git $VCPKG_ROOT \
    && $VCPKG_ROOT/bootstrap-vcpkg.sh -disableMetrics

WORKDIR /src

COPY vcpkg.json ./
RUN $VCPKG_ROOT/vcpkg install --x-install-root=/src/vcpkg_deps

COPY . .
RUN cmake --preset release -DVCPKG_INSTALLED_DIR=/src/vcpkg_deps \
    && cmake --build --preset release --target discord_bot

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates python3 python3-venv \
    && rm -rf /var/lib/apt/lists/*

RUN python3 -m venv /opt/yt-dlp \
    && /opt/yt-dlp/bin/pip install --no-cache-dir "yt-dlp[default]"
ENV PATH="/opt/yt-dlp/bin:${PATH}"

COPY --from=denoland/deno:bin /deno /usr/local/bin/deno

COPY --from=build /src/build/release/discord_bot /usr/local/bin/discord_bot
COPY --from=build /src/vcpkg_deps/x64-linux/lib/libdpp.so* /usr/local/lib/
RUN ldconfig

RUN useradd --create-home --shell /usr/sbin/nologin bot
USER bot
WORKDIR /home/bot

ENTRYPOINT ["discord_bot"]
