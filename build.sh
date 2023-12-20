#!/bin/bash

set -xe

gcc -g3 -Wall -Wextra -pedantic -o modplayer modplayer.c -lxmp
