find . -maxdepth 1 -mindepth 1 -type d | grep -v -E 'arm_lite_codec|lib' | xargs rm -rf
