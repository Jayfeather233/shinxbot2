#!/bin/bash

apt-get update
apt-get install -y build-essential wget cmake 
apt-get install -y libcurl4-openssl-dev openssl libssl-dev libjsoncpp-dev libzip-dev
apt-get install -y libjpeg-dev libpng-dev libtiff-dev libgif-dev libwebp-dev webp libmagick++-dev
apt-get install -y libfmt-dev

cd /tmp
wget https://www.imagemagick.org/download/ImageMagick.tar.gz
tar -xvf ImageMagick.tar.gz
cd ImageMagick-* && ./configure && make && make install
echo "/usr/local/lib/" >> /etc/ld.so.conf && ldconfig

cd /workspace
./build.sh

tail -f /dev/null