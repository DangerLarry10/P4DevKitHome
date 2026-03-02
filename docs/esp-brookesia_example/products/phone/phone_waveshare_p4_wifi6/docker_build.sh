#!/bin/bash
# Build script to run inside espressif/idf Docker container
set -e

cd /project/products/phone/phone_waveshare_p4_wifi6

# Set target and build
idf.py set-target esp32p4
idf.py build
