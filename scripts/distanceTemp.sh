#!/bin/env bash
BROKER="192.168.0.183";
PORT=1883
mosquitto_sub -h "$BROKER" -p "$PORT" -v \
 -t "esp32/distance" -t "esp32/tempHum" |
while IFS=' ' read -r topic rest; do
  ts="$(date '+%Y-%m-%d %H:%M:%S')"
  case "$topic" in
     esp32/distance) echo "$ts $rest" >> distance.log ;;
     esp32/tempHum)  echo "$ts $rest" >> tempHUM.log  ;;
  esac
done
