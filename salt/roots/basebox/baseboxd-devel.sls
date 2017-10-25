include:
  - .repos

basebox.build.pkg:
  pkg.installed:
    - pkgs:
      - libnl3-devel
      - rofl-common-devel
      - rofl-ofdpa-devel
      - glog-devel
      - gflags-devel
      - grpc-devel
      - grpc-plugins.x86_64
    - require:
      - pkgrepo: bisdn-baseboxd
      - pkgrepo: bisdn-rofl
