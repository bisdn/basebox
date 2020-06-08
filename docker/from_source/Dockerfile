FROM fedora:30
LABEL maintainer="jan.klare@bisdn.de"
ADD . / basebox/
RUN dnf -y install dnf-plugins-core && \
    dnf -y install git && \
    dnf -y install clang && \
    dnf -y install make && \
    dnf -y install meson && \
    dnf -y install ninja-build && \
    dnf -y install gettext && \
    dnf -y install findutils && \
    dnf -y install systemd-devel && \
    dnf -y copr enable bisdn/rofl && \
    dnf -y copr enable bisdn/rofl-testing && \
    dnf -y copr enable bisdn/baseboxd && \
    dnf -y copr enable bisdn/baseboxd-testing && \
    cd basebox && \
    git submodule update --init --recursive && \
    make -C ./pkg/testing/rpm/ spec && \
    dnf -y builddep ./pkg/testing/rpm/baseboxd.spec && \
    meson build && \
    ninja -C build && \
    cp build/baseboxd /usr/bin/baseboxd && \
    dnf -y remove dnf-plugins-core && \
    dnf -y remove git && \
    dnf -y remove clang && \
    dnf -y remove make && \
    dnf -y remove meson && \
    dnf -y remove ninja-build && \
    dnf -y remove gettext && \
    dnf -y remove findutils && \
    dnf autoremove && \
    dnf clean all --enablerepo=\* && \
    cd .. && \
    rm -rf baseboxd
CMD /usr/bin/baseboxd
