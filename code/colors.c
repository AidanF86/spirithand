#include <SDL2/SDL.h>

global_variable SDL_Color color_red;
global_variable SDL_Color color_orange;
global_variable SDL_Color color_yellow;
global_variable SDL_Color color_green;
global_variable SDL_Color color_blue;
global_variable SDL_Color color_purple;
global_variable SDL_Color color_black;
global_variable SDL_Color color_white;
global_variable SDL_Color color_grass_green;

void initcolors()
{
    color_red.r = 255;
    color_red.g = 0;
    color_red.b = 0;
    color_red.a = 255;
    color_orange.r = 255;
    color_orange.g = 128;
    color_orange.b = 0;
    color_orange.a = 255;
    color_yellow.r = 255;
    color_yellow.g = 255;
    color_yellow.b = 0;
    color_yellow.a = 255;
    color_green.r = 128;
    color_green.g = 255;
    color_green.b = 0;
    color_green.a = 255;
    color_blue.r = 0;
    color_blue.g = 0;
    color_blue.b = 255;
    color_blue.a = 255;
    color_purple.r = 255;
    color_purple.g = 0;
    color_purple.b = 255;
    color_purple.a = 255;
    color_white.r = 255;
    color_white.g = 255;
    color_white.b = 255;
    color_white.a = 255;

    color_grass_green.r = 72;
    color_grass_green.g = 199;
    color_grass_green.b = 24;
    color_grass_green.a = 255;
}

