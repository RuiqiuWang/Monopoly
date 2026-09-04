#!/usr/bin/env bash
set -euo pipefail

make clean CC=gcc
make game_engine CC=gcc

make test CC=gcc
make json_test CC=gcc
