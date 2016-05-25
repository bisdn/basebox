# -*- mode: ruby -*-

NAME = 'basebox'

# If you are using Virtualbox, please make sure you have the vbguest plugin installed.
#
# We are bootstrapping salt via bash and don't use Vagrant's provisioning.
# The recommended Fedora box from getfedora.com does not support this.
#
# Tom decided to go with a custom installation of salt (and not using the vagrant salt provision), due to the following problems:
# - salt, we need > 2015.8.3 otherwise it will crash due to the move to dnf
# - salt, the bootstrap-script downloads a .repo file into /etc/yum.repos... The URL used however only contains a redirect (and hence html instead of the actual config is written). This breaks dnf.
# - salt, on fedora 22, fails with CommandExecutionError: Error: unrecognized arguments: --installed usage: dnf repoquery
# - salt, in 2015.5.9-2, we get "The following packages failed to install/update" due to some issue with 'dnf repoquery --quiet --queryformat'. This can be solved by running "dnf update" before running salt

# To test provisining inside the VMto test you can use:
#   sudo salt-call --local -l debug --file-root=/vagrant/salt/ state.highstate
#   sudo salt-call --local -l debug --file-root=/vagrant/salt/ state.sls build
#   dnf -y --enablerepo=updates-testing install salt-minion

SALT = <<SCRIPT
dnf -y update
dnf -y --enablerepo updates-testing install salt-minion
dnf -y install git # we also need git to make salt succeed
salt-call --local --file-root /vagrant/salt/ state.sls build
salt-call --local --file-root /vagrant/salt/ state.sls repos
salt-call --local --file-root /vagrant/salt/ state.sls car-devel
SCRIPT

Vagrant.configure("2") do |config|
  config.vm.hostname = NAME
  config.vm.box = "fedora/23-cloud-base"
  config.vm.provision :shell, inline: SALT, keep_color: true
  config.vm.network "forwarded_port", guest: 6653, host: 6653

  config.vm.provider "virtualbox" do |vb, override|
    override.vm.synced_folder ".", "/vagrant", type: "virtualbox"
    vb.name = NAME
    vb.memory = 1024
    vb.cpus = 1
  end

  config.vm.provider :libvirt do |libvirt, override|
    override.vm.synced_folder "./", "/vagrant", { nfs: true }
    libvirt.memory = 1024
    libvirt.cpus = 1
  end
end
