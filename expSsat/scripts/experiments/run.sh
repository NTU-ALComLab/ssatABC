#!/bin/bash
./run-erssat.sh "" ToiletA expSsat/ssatER/planning/ToiletA/ &
./run-erssat.sh "" conformant expSsat/ssatER/planning/conformant/ &
./run-random-erssat.sh "" &

./run-erssat.sh -pgs ToiletA expSsat/ssatER/planning/ToiletA/ &
./run-erssat.sh -pgs conformant expSsat/ssatER/planning/conformant/ &
./run-random-erssat.sh -pgs &

./run-dcssat.sh ToiletA-conformant all_ai_ssat/ &
./run-random-dcssat.sh &
