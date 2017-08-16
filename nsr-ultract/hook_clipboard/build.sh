#!/bin/bash

gcc `pkg-config gtk+-2.0 --cflags` hooker.c -o hooker.so -shared -ldl -fPIC
