include:
  - repos

basebox.build.pkg:
  pkg.installed:
    - pkgs:
      - libnl3
      - libnl3-devel
    - require:
      - pkgrepo: BISDN-priv
      - pkgrepo: toanju-rofl-common

car.build.pkg:
  pkg.installed:
    - pkgs:
      - libconfig-devel
      - rofl-common-devel
      - rofl-common-debuginfo
      - rofl-ofdpa-devel
      - rofl-ofdpa-debuginfo
    - require:
      - pkgrepo: BISDN-priv
      - pkgrepo: toanju-rofl-common

car.test.pkg:
  pkg.installed:
    - pkgs:
      - gtest
      - gtest-devel
      - gmock
      - gmock-devel

car.test.gmock.lib:
  cmd.run:
    - names:
      - g++ -isystem -I/usr/include -isystem /usr/include/gmock -pthread /usr/src/gmock/gmock-all.cc -c -o /usr/src/gmock/gmock-all.o
      - ar -rv /usr/lib/libgmock.a /usr/src/gmock/gmock-all.o
    - unless: test -e /usr/lib/libgmock.a
    - require:
      - pkg: car.test.pkg

car.style.pkg:
  pkg.installed:
    - pkgs:
      - clang

car.doc.pkg:
  pkg.installed:
    - pkgs:
      - doxygen

car.convenience.pkg:
  pkg.installed:
    - pkgs:
      - vim-enhanced

# fix locale warnings
us_locale:
  locale.present:
    - name: en_US.UTF-8

default_locale:
  locale.system:
    - name: en_US.UTF-8
    - require:
      - locale: us_locale
