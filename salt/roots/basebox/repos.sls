yum:
  pkg.installed

toanju-baseboxd:
  pkgrepo.managed:
    - humanname: Copr repo for baseboxd owned by toanju
    - baseurl: https://copr-be.cloud.fedoraproject.org/results/toanju/baseboxd/fedora-$releasever-$basearch/
    - skip_if_unavailable: True
    - gpgcheck: 1
    - gpgkey: https://copr-be.cloud.fedoraproject.org/results/toanju/baseboxd/pubkey.gpg
    - enabled: 1
    - enabled_metadata: 1
    - require:
      - pkg: yum

toanju-rofl-common:
  pkgrepo.managed:
    - humanname: Copr repo for rofl-common owned by toanju
    - baseurl: https://copr-be.cloud.fedoraproject.org/results/toanju/rofl-common/fedora-$releasever-$basearch/
    - skip_if_unavailable: True
    - gpgcheck: 1
    - gpgkey: https://copr-be.cloud.fedoraproject.org/results/toanju/rofl-common/pubkey.gpg
    - enabled: 1
    - enabled_metadata: 1
    - require:
      - pkg: yum
