#!/bin/bash

echo $1

tr "," " " < $1| tail -n+3 | sox  -r 10000 -c 1 -t dat - -r 16000 -t dat - sinc 1500 -n 1024 | tail -n+3 | ./demod > /tmp/temp_neu.dat # | sox -S -r 8000 -t dat -c 2 - /tmp/temp.wav 
