FROM fedora:30
LABEL maintainer="jan.klare@bisdn.de"
RUN dnf -y install dnf-plugins-core && \
    dnf -y copr enable bisdn/rofl && \
    dnf -y copr enable bisdn/baseboxd && \
    dnf -y install baseboxd && \
    dnf -y remove dnf-plugins-core && \
    dnf autoremove && \
    dnf clean all --enablerepo=\*
CMD /usr/bin/baseboxd
