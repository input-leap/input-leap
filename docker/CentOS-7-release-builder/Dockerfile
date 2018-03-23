FROM centos:centos7

RUN yum -y update && yum -y upgrade
RUN yum -y install epel-release
RUN yum -y install cmake3 make qt5-qtbase-devel gcc \
 avahi-compat-libdns_sd-devel gcc-c++ curl-devel \
 openssl-devel \
 libXtst-devel libXinerama-devel libICE-devel libXrandr-devel \
 wget rpm-build 

# during the testing period using truatpasteurdotfr/barrier pre-released tarball
# final version should use the https://github.com/debauchee/barrier release versions
RUN wget https://github.com/truatpasteurdotfr/barrier/archive/v2.0.0-RC2.tar.gz  -o barrier-2.0.0-Source.tar.gz  && rpmbuild -ta barrier-2.0.0-Source.tar.gz  | tee rpmbuild.log
