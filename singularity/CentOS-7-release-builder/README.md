building the container:
```
sudo singularity build CentOS-7-release-builder.simg Singularity 
```

running it to build the rpm/deb
```
export OUT=`mktemp -d` && echo building in ${OUT} && \
singularity run -H ${OUT}  CentOS-7-release-builder.simg
```

use the container to build another version:
```
export OUT=`mktemp -d` && echo building in ${OUT} && \
singularity exec -H ${OUT}  CentOS-7-release-builder.simg bash -c "wget https://github.com/truatpasteurdotfr/barrier/archive/v2.0.0-RC2.tar.gz   && rpmbuild -ta v2.0.0-RC2.tar.gz"

```

