include:
  - .repos

basebox.build.pkg:
  pkg.installed:
    - pkgs:
      - libnl3-devel
      - rofl-common-devel
      - rofl-ofdpa-devel
    - require:
      - pkgrepo: bisdn-baseboxd
      - pkgrepo: bisdn-rofl
