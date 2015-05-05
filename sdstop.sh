#!/bin/sh

for pid in $(ps -le | awk '/socketdaemon/ {print $4}'); do kill -9 $pid; done
