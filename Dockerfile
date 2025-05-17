FROM debian:12

RUN apt-get update && apt-get install -y build-essential wget cmake \
    libcurl4-openssl-dev openssl libssl-dev libjsoncpp-dev libzip-dev \
    libjpeg-dev libpng-dev libtiff-dev libgif-dev libwebp-dev webp \ 
    libmagick++-dev libfmt-dev python3

RUN wget https://www.imagemagick.org/download/ImageMagick.tar.gz -P /tmp && \
    cd /tmp && tar -xvf ImageMagick.tar.gz && \
    cd ImageMagick-* && ./configure && make && make install && \
    echo "/usr/local/lib/" >> /etc/ld.so.conf && ldconfig
