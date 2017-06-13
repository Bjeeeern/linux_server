#!/bin/bash

domain=http://localhost:8000
#domain=http://78.69.79.63:8000
main_page_name="Main Page"
src="../code"
jssettings="--javascript-delay 4000 --no-stop-slow-scripts --enable-javascript --debug-javascript"

rm "$main_page_name.png" &
wkhtmltoimage $jssettings --width 1200 --height 800 --quality 10 "$domain/index.html" "$main_page_name.png"
# xvfb wkhtmltoimage --width 1200 --height 800 --quality 10 "$domain/index.html" "$main_page_name.png"
convert -resize 20% "$main_page_name.png" "$main_page_name.png"

while read name
do
    url="$domain$name"
    echo $url
    sub_page=$(sed "s:/::"  <<< "$name")
    echo $sub_page
    rm "$sub_page.png" &
    wkhtmltoimage $jssettings --width 1200 --height 800 --quality 10 "$url" "$sub_page.png"
    # xvfb wkhtmltoimage --width 1200 --height 800 --quality 10 "$url" "$sub_page.png"
    convert -resize 20% "$sub_page.png" "$sub_page.png"
done < <(find $src -mindepth 1 -type d | sed "s:$src::")
