# -*- mode: ruby -*-

# Sometimes the boot stalls. This is due to cloud-init forcing the network to come up. This has a time out of ~ 2-3 minutes. Just wait...

NAME = "baseboxd"

MIRRORS = <<SCRIPT
echo "###### Ubuntu Main Repos
deb http://de.archive.ubuntu.com/ubuntu/ trusty main restricted universe multiverse
deb-src http://de.archive.ubuntu.com/ubuntu/ trusty main restricted universe multiverse

###### Ubuntu Update Repos
deb http://de.archive.ubuntu.com/ubuntu/ trusty-security main restricted universe multiverse
deb http://de.archive.ubuntu.com/ubuntu/ trusty-updates main restricted universe multiverse
deb http://de.archive.ubuntu.com/ubuntu/ trusty-proposed main restricted universe multiverse
deb http://de.archive.ubuntu.com/ubuntu/ trusty-backports main restricted universe multiverse
deb-src http://de.archive.ubuntu.com/ubuntu/ trusty-security main restricted universe multiverse
deb-src http://de.archive.ubuntu.com/ubuntu/ trusty-updates main restricted universe multiverse
deb-src http://de.archive.ubuntu.com/ubuntu/ trusty-proposed main restricted universe multiverse
deb-src http://de.archive.ubuntu.com/ubuntu/ trusty-backports main restricted universe multiverse" > /etc/apt/sources.list
SCRIPT

UPGRADE = <<SCRIPT
# Update system
apt-get -y update
apt-get -y dist-upgrade
apt-get -y upgrade
SCRIPT

DEVBASE = <<SCRIPT
apt-get -y install git vim man wget
apt-get -y install build-essential dh-autoreconf pkg-config
SCRIPT

LIBCONFIG = <<SCRIPT
apt-get -y install libconfig++-dev
SCRIPT

LIBNL = <<SCRIPT
apt-get -y install bison flex
pushd /opt
git clone https://github.com/thom311/libnl.git # I tried it out with commit eaa75b7c7d3e6a4df1a2e7591ae295acfae3f73e
cd libnl
./autogen.sh
./configure
make && make install
ldconfig
SCRIPT

ROFL = <<SCRIPT
pushd /opt
git clone -b integration-0.7 --depth 1 https://github.com/bisdn/rofl-common.git
cd rofl-common
./autogen.sh
cd build
../configure
make && make install
SCRIPT

Vagrant.configure("2") do |config|
  config.vm.box = "ubuntu/trusty64"
  config.vm.hostname = NAME

  config.vm.provision :shell, inline: MIRRORS, keep_color: true
  config.vm.provision :shell, inline: UPGRADE, keep_color: true
  config.vm.provision :shell, inline: DEVBASE, keep_color: true
  config.vm.provision :shell, inline: LIBCONFIG, keep_color: true
  config.vm.provision :shell, inline: LIBNL, keep_color: true
  config.vm.provision :shell, inline: ROFL, keep_color: true

  config.vm.network "forwarded_port", guest: 6653, host: 6653

  config.vm.provider "virtualbox" do |vb|
    vb.name = NAME
    vb.memory = 1024
    vb.gui = false
  end
end