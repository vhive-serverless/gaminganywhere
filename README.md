GamingAnywhere
==============

GamingAnywhere: An Open Cloud Gaming System

# Prerequisite knowledge
1. RTSP and RTP for network communication
2. RGB and YUV image formats and conversion
3. Marshalling and unmarshalling

# Overview

GamingAnywhere is an open-source clouding gaming platform. In addition to its
openness, we design GamingAnywhere for high extensibility, portability, and
reconfigurability. GamingAnywhere currently supports Windows and Linux, and
can be ported to other OS's including OS X and Android.

# Documents

* Official web site: http://gaminganywhere.org/

* Quick start guide: http://gaminganywhere.org/doc/quick_start.html

* Configuration file guide: http://gaminganywhere.org/doc/config.html

* FAQ: http://gaminganywhere.org/faq.html

# Quick Notes

* Recommended development platforms Ubuntu Linux x86_64.

* Required packages on Linux OS (both runtime and development files):
```libx11```, ```libxext```, ```libxtst```, ```libfreetype6```,
```libgl1-mesa```, ```libglu1-mesa```, ```libpulse```,
```libasound2```, ```lib32z1```

* Sample command to install required packages on Ubuntu Linux:
  ```
  apt-get  install patch make cmake g++ pkg-config \
		libx11-dev libxext-dev libxtst-dev libfreetype6-dev \
		libgl1-mesa-dev libglu1-mesa-dev \
		libpulse-dev libasound2-dev lib32z1
  ```

# Settings
In all Dockerfile, change "/users/yankai14/gaminganywhere" to your own gaminganywhere directory.
In env-setup change your LD_LIBRARY_PATH to the correct path that points to deps.posix/lib.

# To run headless
Compile binaries
```
sudo ./setup.sh
```

Run client
```
./run-client.sh
```

Run server
```
./run-server.sh
```

# Run servers on docker-compose
```
docker-compose build
docker-compose up -d
```

Then trigger using ./run-client.sh

# Run servers on knative
```
kubectl apply -f kn-gaminganywhere-filter.yml
kubectl apply -f kn-gaminganywhere-vsource.yml
```
Modify ./run-client.sh ./ga-client config/client.rel.conf rtsp://<host>:<port>/desktop
Then trigger using ./run-client.sh