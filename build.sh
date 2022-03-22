#!/bin/sh
pushd ~/projects/spirithand
gcc -g code/spirithand.c `sdl2-config --libs` -lSDL2 -lSDL2_image -lSDL2_ttf -o spirithand -lm
