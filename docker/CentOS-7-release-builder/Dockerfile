FROM centos:centos7

RUN yum -y update && yum -y upgrade
RUN yum -y install epel-release
RUN yum -y install cmake3 make qt5-qtbase-devel gcc \
 avahi-compat-libdns_sd-devel gcc-c++ curl-devel \
 openssl-devel \
 libXtst-devel libXinerama-devel libICE-devel libXrandr-devel \
 wget rpm-build alien

# during the testing period using truatpasteurdotfr/barrier pre-released tarball
# final version should use the https://github.com/debauchee/barrier release versions
RUN wget https://github.com/truatpasteurdotfr/barrier/archive/v2.0.0-RC2.tar.gz  && rpmbuild -ta v2.0.0-RC2.tar.gz  | tee rpmbuild.log
# quick and dirty conversion to debian, should be ok for stretch amd64 
RUN alien -d /root/rpmbuild/RPMS/x86_64/barrier-2.0.0-1.el7.centos.x86_64.rpm 
RUN tar czvf rpmbuild.tgz /root/rpmbuild rpmbuild.log barrier_*.deb
