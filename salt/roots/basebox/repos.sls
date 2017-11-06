yum:
  pkg.installed

bisdn-baseboxd:
  pkgrepo.managed:
    - humanname: Copr repo for baseboxd owned by bisdn
    - baseurl: https://copr-be.cloud.fedoraproject.org/results/bisdn/baseboxd/fedora-$releasever-$basearch/
    - skip_if_unavailable: True
    - gpgcheck: 1
    - gpgkey: https://copr-be.cloud.fedoraproject.org/results/bisdn/baseboxd/pubkey.gpg
    - enabled: 1
    - enabled_metadata: 1
    - require:
      - pkg: yum

bisdn-rofl:
  pkgrepo.managed:
    - humanname: Copr repo for rofl owned by bisdn
    - baseurl: https://copr-be.cloud.fedoraproject.org/results/bisdn/rofl/fedora-$releasever-$basearch/
    - skip_if_unavailable: True
    - gpgcheck: 1
    - gpgkey: https://copr-be.cloud.fedoraproject.org/results/bisdn/rofl/pubkey.gpg
    - enabled: 1
    - enabled_metadata: 1
    - require:
      - pkg: yum

bisdn-baseboxd-testing-copr:
  pkgrepo.managed:
    - name: bisdn-baseboxd-testing
    - humanname: Copr repo for baseboxd-testing owned by bisdn
    - baseurl: https://copr-be.cloud.fedoraproject.org/results/bisdn/baseboxd-testing/fedora-$releasever-$basearch/
    - type: rpm-md
    - skip_if_unavailable: True
    - gpgcheck: 1
    - gpgkey: https://copr-be.cloud.fedoraproject.org/results/bisdn/baseboxd-testing/pubkey.gpg
    - repo_gpgcheck: 0
    - enabled: 1
    - enabled_metadata: 1
