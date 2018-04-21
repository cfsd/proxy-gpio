
docker build -t proxy-lynx-gpio-v0.0.1 -f Dockerfile.armhf .
docker save  proxy-lynx-gpio-v0.0.1 > proxy-lynx-gpio-v0.0.1.tar
