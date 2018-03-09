FROM debian:9
## root@765ebcab56d1:~# cat /etc/apt/sources.list
## #deb http://deb.debian.org/debian stretch main
## #deb http://deb.debian.org/debian stretch-updates main
## deb http://cdn-fastly.deb.debian.org/debian stretch main
## deb http://cdn-fastly.deb.debian.org/debian stretch-updates main
## deb http://security.debian.org stretch/updates main
## 
RUN sed -i -e 's,http://deb.debian.org,http://cdn-fastly.deb.debian.org,g' /etc/apt/sources.list
RUN apt-get update && apt-get -y upgrade
RUN apt-get -y install \
build-essential cmake qt5-default \
git \
libxtst-dev libxinerama-dev libice-dev libxrandr-dev \
libavahi-compat-libdnssd-dev \
libcurl4-openssl-dev \
libssl1.0-dev
#libssl-dev  is causing these warnings -> using libssl1.0-dev instead
#/usr/bin/ld: warning: libssl.so.1.0.2, needed by /usr/lib/gcc/x86_64-linux-gnu/6/../../../x86_64-linux-gnu/libcurl.so, may conflict with libssl.so.1.1
#/usr/bin/ld: warning: libcrypto.so.1.0.2, needed by /usr/lib/gcc/x86_64-linux-gnu/6/../../../x86_64-linux-gnu/libcurl.so, may conflict with libcrypto.so.1.1
#
RUN git clone https://github.com/debauchee/barrier/
RUN cd barrier && ./clean_build.sh | tee clean_build.log && tar czvf build.tgz build clean_build.log

