FROM centos:centos7

RUN yum -y update && yum -y upgrade
RUN yum -y install epel-release
RUN yum -y install git cmake3 make qt5-qtbase-devel gcc \
 avahi-compat-libdns_sd-devel gcc-c++ curl-devel \
 openssl-devel \
 libXtst-devel libXinerama-devel libICE-devel libXrandr-devel

RUN git clone https://github.com/debauchee/barrier/
# pending a PR since which is not installed by default on the CentOS-7 docker image
# `type cmake3 >> /dev/null` && B_CMAKE=`type cmake3 | cut -d ' ' -f 3` instead of B_CMAKE=`which cmake3 2>/dev/null`
RUN yum -y install which 
RUN cd barrier && ./clean_build.sh | tee clean_build.log && tar czvf build.tgz build clean_build.log
