# server
Client server using TCP/IP

I have used Multithreaded, libevent-based socket server which is open source (BSD License).
I've done appropriate modifications in that.

The below utility has been built and tested on Ubuntu 14.04.1 (kernel: 3.13.0-32-generic)
Building the server program:
============================
There is a Makefile available to build the server program.
Just run make command to build the server daemon.
Output file "socketdaemon" shall be created.
"$make clean" can be used to clean the build.

Setting up the server as daemon:
================================
### NOTE: you must have root permission to do this ###

In order to setup this server program as daemon run the configure script from command prompt e.g.
$./configure.sh
This will setup the configure script to run automatically when the system boots up.
the server can also be controlled (start/stop/restart) as follows
/etc/init.d/sockd stop
/etc/init.d/sockd start
/etc/init.d/sockd restart

Client:
=======
The client program is written in python
File: client.py
To run the client use command as follows:
$python client.py <IP address> <port number> <number of requests> 
for example:
$python client.py 192.168.0.1 8787 5

The response shall be printed on terminal as well as saved in the log file (/var/tmp/sockd.log)
