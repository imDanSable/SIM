#!/usr/bin/env bash

cd ../../../
rm ./screenshots/SIM/*.png
./Rack -t 8 -d
src_dir="./screenshots/SIM/"
dest_dir="./plugins/SIM/res/manual/"
for file in ${src_dir}*.png; do
    filename=$(basename -- "$file")
    convert "$file" -resize 15% -filter Lanczos -quality 100% "${dest_dir}${filename}"
done