FROM debian:12

ARG IMAGEMAGICK_VERSION=7.1.2-18

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    pkg-config \
    python3 \
    curl \
    xz-utils \
    ca-certificates \
    libcurl4-openssl-dev \
    openssl \
    libssl-dev \
    libjsoncpp-dev \
    libzip-dev \
    libjpeg-dev \
    libpng-dev \
    libtiff-dev \
    libgif-dev \
    libwebp-dev \
    webp \
        libxml2-dev \
        libltdl-dev \
    libfmt-dev && \
    rm -rf /var/lib/apt/lists/*

# Build ImageMagick 7 because Debian 12 apt provides ImageMagick 6 by default.
RUN set -eux; \
        cd /tmp; \
        curl -fsSLO "https://imagemagick.org/archive/releases/ImageMagick-${IMAGEMAGICK_VERSION}.tar.xz"; \
        tar -xJf "ImageMagick-${IMAGEMAGICK_VERSION}.tar.xz"; \
        cd "ImageMagick-${IMAGEMAGICK_VERSION}"; \
        ./configure \
            --prefix=/usr/local \
            --with-modules \
            --with-magick-plus-plus=yes \
            --disable-static; \
        make -j"$(nproc)"; \
        make install; \
        ldconfig; \
        magick -version; \
        rm -rf /tmp/ImageMagick-* /tmp/*.tar.xz
