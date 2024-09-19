#!/bin/sh
timeout 20 ./a.out 1;   sleep 5  # send at 1 MBps
timeout 20 ./a.out 1.5; sleep 5  # send at 1.5 MBps
timeout 20 ./a.out 2;   sleep 5  # send at 2 MBps
timeout 20 ./a.out 3
