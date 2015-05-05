# Copyright (c) 2015 Sandeep Panwar
# This software is licensed under the Free BSD license.
# See the accompanying LICENSE.txt for details.
 
import socket
import sys
import logging

args = len(sys.argv)
PORT = 5555
reqn = 1

if args != 4:
    print "Usage: python client.py <Server IP address> <port> <No of request to send>"
    sys.exit()
else:
    try:
        socket.inet_aton(sys.argv[1])
    except socket.error:
        print "Not a valid IP Address"
        sys.exit()
    HOST = sys.argv[1]
    try:
        PORT = int(sys.argv[2])
        reqn = int(sys.argv[3])
    except ValueError:
        print "Invalid port number OR Number of request"
        sys.exit()

logger = logging.getLogger('sockd')
hdlr = logging.FileHandler('/var/tmp/sockd.log')
formatter = logging.Formatter('%(asctime)s %(levelname)s %(message)s')
hdlr.setFormatter(formatter)
logger.addHandler(hdlr) 
logger.setLevel(logging.INFO)

for i in range(0, reqn):
    data = "GETMESSAGE\n"
    try:
        # Create a socket (SOCK_STREAM means a TCP socket)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # Connect to server and send data
        sock.connect((HOST, PORT))
        sock.send(data + "\n")
    except:
        print "Error in communicating with server !"
        logger.error("Error in communicating with server")
        break


    # Receive data from the server and shut down
    received = ""

    try:
        received = sock.recv(1024)
        sock.close()
        
        print "Sent:     %s" % data
        print "Received: %s" % received
        logger.info(received)
    except:
        print "Error while receiving data from server !"
        break
