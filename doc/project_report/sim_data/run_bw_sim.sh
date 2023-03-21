#!/bin/bash
loop_count=20
n=0

while [ $n -le $loop_count ]
do
    google-chrome --new-window http://localhost:8080/control &
    sleep 10
    ((n++))
done
