include:
  - .repos

basebox.build.pkg:
  pkg.installed:
    - pkgs:
      - gflags-devel
      - glog-devel
      - grpc-devel
      - grpc-plugins
      - libnl3-devel
      - protobuf-devel
      - rofl-common-devel
      - rofl-ofdpa-devel
    - require:
      - pkgrepo: bisdn-baseboxd
      - pkgrepo: bisdn-rofl
