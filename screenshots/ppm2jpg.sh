#!/bin/bash

if ! command -v magick &> /dev/null
then
    echo "imagemagick not installed"
else
    for i in *.ppm; do
        filename=${i%.*}
        magick "$filename.ppm" +level-colors Grey20,Orange1 -resize 512x384! -sharpen 0x2.0 "$filename.jpg"
    done
fi
