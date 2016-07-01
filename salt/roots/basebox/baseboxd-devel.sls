include:
  - .repos

basebox.build.pkg:
  pkg.installed:
    - pkgs:
      - libnl3-devel
      - rofl-common-devel
      - rofl-ofdpa-devel
    - require:
      - pkgrepo: toanju-baseboxd
      - pkgrepo: toanju-rofl-common
