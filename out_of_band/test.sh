#!/bin/bash
ARG_P=5
ARG_K=8
ARG_W=20

unlink supervisor.log 2>/dev/null
unlink client.log 2>/dev/null
./supervisor $ARG_K 1>>$2 &
sleep 2
for i in {1..10}; do
  ./client $ARG_P $ARG_K $ARG_W 1>>$1 &
  ./client $ARG_P $ARG_K $ARG_W 1>>$1 &
  sleep 1
done

for j in {1..6}; do
  sleep 10
  pkill -INT supervisor
done
pkill -INT supervisor
