/etc/hosts:
  file.append:
    - text: '172.30.10.11 ftp.bisdn.de'
    - require_in:
      - pkgrepo: BISDN-priv
