# -*- mode: ruby -*-
# If you are using Virtualbox, please make sure you have the vbguest plugin installed.
NAME = 'basebox'

Vagrant.configure("2") do |config|
  config.vm.hostname = NAME
  config.vm.box = "fedora/28-cloud-base"
  config.vm.network "forwarded_port", guest: 6653, host: 6653
  config.vm.synced_folder "salt/roots/", "/srv/salt/", type: "nfs", nfs_version: 3, nfs_udp: false

  config.vm.provision :salt do |salt|
    salt.masterless = true
    salt.minion_config = "salt/minion"
    salt.run_highstate = true
  end

  config.vm.provider "virtualbox" do |vb, override|
    override.vm.synced_folder ".", "/vagrant", type: "virtualbox"
    vb.name = NAME
    vb.memory = 1024
    vb.cpus = 2
  end

  config.vm.provider :libvirt do |libvirt, override|
    override.vm.synced_folder "./", "/vagrant", type: "nfs", nfs_version: 3, nfs_udp: false
    libvirt.memory = 1024
    libvirt.cpus = 2
  end
end
