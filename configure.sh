#!/bin/sh

sudo mkdir /var/socketdaemon
sudo cp ./sockd /var/socketdaemon
sudo cp socketdaemon /var/socketdaemon
sudo cp ./sdstop.sh /var/socketdaemon

sudo cp ./sockd /etc/init.d/
sudo chmod 755 /etc/init.d/sockd
cd /etc/rc0.d
sudo ln -s ../init.d/sockd K20sockd
cd /etc/rc1.d
sudo ln -s ../init.d/sockd K20sockd
cd /etc/rc2.d
sudo ln -s ../init.d/sockd S20sockd
cd /etc/rc3.d
sudo ln -s ../init.d/sockd S20sockd
cd /etc/rc4.d
sudo ln -s ../init.d/sockd S20sockd
cd /etc/rc5.d
sudo ln -s ../init.d/sockd S20sockd
cd /etc/rc6.d
sudo ln -s ../init.d/sockd K20sockd
