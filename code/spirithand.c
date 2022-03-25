#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#define internal static
#define local_persist static
#define global_variable static

#include "colors.c"
#include "mathandphysics.c"

bool handleevent(SDL_Event *event);

typedef struct
Spirit {
    double x; double y;
    double velx; double vely;
    Box box;
    SDL_Color color;
} Spirit;

typedef struct
Debuginfo {
    int colliders;
    SDL_Color collidercolor;
    int worldgrid;
    SDL_Color worldgridcolor;
    int fps;
} Debuginfo;

typedef struct
Keysdown {
    bool up;
    bool down;
    bool left;
    bool right;
} Keysdown;

global_variable Debuginfo debug;
global_variable Keysdown keysdown;
global_variable Camera maincam;
global_variable Vectorf mousepos;

#define MAXSPIRITS 54
global_variable int spiritcount;
global_variable Spirit spirits[MAXSPIRITS]; // idk 54 kinda sounds and feels right


Spirit*
makespirit(double x, double y, double size, SDL_Color color)
{
    if(spiritcount == MAXSPIRITS)
    {
        printf("makespirit: spiritcount is already at MAXSPIRITS!\n");
        return NULL;
    }
    Spirit newspirit;
    newspirit.x = x;
    newspirit.y = y;
    newspirit.box.x = newspirit.x; - size / 2.0;
    newspirit.box.y = newspirit.y; - size / 2.0;
    newspirit.box.w = size;
    newspirit.box.h = size;
    newspirit.color = color;
    spirits[spiritcount] = newspirit;
    return &(spirits[spiritcount++]);
}

int
updatespirits()
{
    Spirit* spirit;
    for(int i = 0; i < spiritcount; i++)
    {
        spirit = &(spirits[i]);
        spirit->x += spirit->velx;
        spirit->y += spirit->vely;
        spirit->box.x = spirit->x - spirit->box.w / 2.0;
        spirit->box.y = spirit->y - spirit->box.h / 2.0;
        Box cambox = camtobox(maincam);
        if(!boxtotallyinbox(spirit->box, camtobox(maincam)))
        {
            spirit->velx *= -1.0;
            spirit->vely *= -1.0;
        }
    }
}

int
debugrenderbox(Camera cam, Box box)
{
    SDL_Rect rect = boxtoscreenspace(cam, box.x, box.y, box.w, box.h);
    /*
    SDL_SetRenderDrawColor(cam.renderer,
                           debug.collidercolor.r,
                           debug.collidercolor.g,
                           debug.collidercolor.b,
                           debug.collidercolor.a);
                           */
    SDL_RenderDrawRect(cam.renderer, &rect);
}

int
debugrenderworldgrid(Camera cam)
{
    int minx = cam.x - cam.w / 2;
    int miny = cam.y - cam.h / 2;
    int maxx = cam.x + cam.w / 2;
    int maxy = cam.y + cam.h / 2;
    SDL_SetRenderDrawColor(cam.renderer,
                           debug.worldgridcolor.r,
                           debug.worldgridcolor.g,
                           debug.worldgridcolor.b,
                           debug.worldgridcolor.a);
    for(int x = minx; x <= maxx; x++)
    {
            SDL_RenderDrawLine(cam.renderer,
                               x,
                               miny,
                               x,
                               maxy);
    }
    for(int y = miny; y <= maxy; y++)
    {
        SDL_RenderDrawLine(cam.renderer,
                           y,
                           minx,
                           y,
                           maxx);
    }
}

int
render(Camera cam)
{
    SDL_SetRenderDrawColor(cam.renderer, 0, 0, 0, 255);
    SDL_RenderClear(cam.renderer);
        // draw background canvas (to make sure of the size)
    SDL_Rect canvasrect = boxtoscreenspace(cam,
                                        -cam.w / 2 + cam.x,
                                        -cam.h / 2 + cam.y,
                                        cam.w,
                                        cam.h);
    SDL_SetRenderDrawColor(cam.renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(cam.renderer, &canvasrect);

    debugrenderworldgrid(cam);

    Spirit spirit;
    for(int i = 0; i < spiritcount; i++)
    {
        spirit = spirits[i];
        if(debug.colliders)
        {
            SDL_SetRenderDrawColor(cam.renderer,
                                   spirit.color.r,
                                   spirit.color.g,
                                   spirit.color.b,
                                   spirit.color.a);
            debugrenderbox(cam, spirit.box);
        }
    }
    rendersidebars(cam);
    SDL_RenderPresent(cam.renderer);
}

int
updatemovement()
{
    float speed = 0.01;
    Vectorf camchange = {0};
    if(keysdown.up)
    {
        camchange.y -= speed;
    }
    if(keysdown.down)
    {
        camchange.y += speed;
    }
    if(keysdown.left)
    {
        camchange.x -= speed;
    }
    if(keysdown.right)
    {
        camchange.x += speed;
    }
    maincam.x += camchange.x;
    maincam.y += camchange.y;
    mousepos.x += camchange.x;
    mousepos.y += camchange.y;
}



int main()
{
    srand(time(0));

    initcolors();

    debug.fps = 0;
    debug.collidercolor.r = 255;
    debug.collidercolor.g = 255;
    debug.collidercolor.b = 255;
    debug.collidercolor.a = 255;
    debug.colliders = 1;
    debug.worldgridcolor.r = 255;
    debug.worldgridcolor.g = 0;
    debug.worldgridcolor.b = 255;
    debug.worldgridcolor.a = 255;

    maincam.x = 0;
    maincam.y = 0;
    maincam.w = 100;
    maincam.h = 100;
    maincam.wres = 640;
    maincam.hres = 480;

    mousepos.x = maincam.x + maincam.w / 2;
    mousepos.y = maincam.y + maincam.h / 2;

    spiritcount = 0;
    for(int i = 0; i < 10; i++)
    {
        Spirit *newspirit = makespirit(randdouble(-40, 40), randdouble(-40, 40), 10, color_orange);
        newspirit->velx = randdouble(-0.02,
                                      0.02);
        newspirit->vely = randdouble(-0.02,
                                      0.02);

    }

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Game Window",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          maincam.wres,
                                          maincam.hres,
                                          SDL_WINDOW_RESIZABLE);

    if(window)
    {
        SDL_Renderer *renderer = SDL_CreateRenderer(window,
                                                   -1,
                                                   0);
        if(renderer)
        {
            maincam.renderer = renderer;

            bool running = true;
            while(running)
            {
                SDL_Event event;
                while(SDL_PollEvent(&event))
                {
                    if(handleevent(&event))
                    {
                        running = false;
                    }
                }
                updatemovement();
                updatespirits();
                render(maincam);
                //sleep(0.0166);
                usleep(166);
            }
        }
    }

}


bool handleevent(SDL_Event *event)
{
    bool shouldquit = false;

    switch(event->type)
    {
        case SDL_QUIT:
        {
            printf("SDL_QUIT\n");
            shouldquit = true;
        } break;
        case SDL_MOUSEMOTION:
        {
            Vectorf mouseposingame = vectorftogame(maincam,
                                                   event->motion.x,
                                                   event->motion.y);
            mousepos.x = mouseposingame.x;
            mousepos.y = mouseposingame.y;
                // TODO(aidan): maybe change this to print floats (limit
                // digits though)
        } break;

        case SDL_KEYDOWN:
        {
            SDL_KeyCode keycode = event->key.keysym.sym;
            if(keycode == SDLK_UP)
            {
                keysdown.up = true;
            }
            else if(keycode == SDLK_DOWN)
            {
                keysdown.down = true;
            }
            else if(keycode == SDLK_LEFT)
            {
                keysdown.left = true;
            }
            else if(keycode == SDLK_RIGHT)
            {
                keysdown.right = true;
            }
        } break;
        case SDL_KEYUP:
        {
            SDL_KeyCode keycode = event->key.keysym.sym;
            bool isdown = (event->key.state == SDL_PRESSED);
            bool wasdown = false;
            if (event->key.state == SDL_RELEASED)
            {
                wasdown = true;
            }
            else if (event->key.repeat != 0)
            {
                wasdown = true;
            }
            if (event->key.repeat == 0)
            {
                if(keycode == SDLK_ESCAPE)
                {
                    printf("ESCAPE: ");
                    if(isdown)
                    {
                        printf("isdown ");
                    }
                    if(wasdown)
                    {
                        printf("wasdown");
                    }
                    printf("\n");
                } 
            }
            if(keycode == SDLK_UP)
            {
                keysdown.up = false;
            }
            else if(keycode == SDLK_DOWN)
            {
                keysdown.down = false;
            }
            else if(keycode == SDLK_LEFT)
            {
                keysdown.left = false;
            }
            else if(keycode == SDLK_RIGHT)
            {
                keysdown.right = false;
            }
        } break;


        case SDL_WINDOWEVENT:
        {
            switch(event->window.event)
            {
                case SDL_WINDOWEVENT_RESIZED:
                {
                    SDL_Window *window = SDL_GetWindowFromID(event->window.windowID);
                    SDL_Renderer *renderer = SDL_GetRenderer(window);
                    //printf("SDL_WINDOWEVENT_RESIZED (%d, %d)\n", event->window.data1, event->window.data2);
                    //SDLResizeTexture(&GlobalBackbuffer, Renderer, event->window.data1, event->window.data2);
                    maincam.wres = event->window.data1;
                    maincam.hres = event->window.data2;
                } break;

                case SDL_WINDOWEVENT_FOCUS_GAINED:
                {
                    printf("SDL_WINDOWEVENT_FOCUS_GAINED\n");
                } break;

                case SDL_WINDOWEVENT_EXPOSED:
                {
                    //SDL_Window *window = SDL_GetWindowFromID(event->window.windowID);
                    //SDL_Renderer *renderer = SDL_GetRenderer(window);
                    //SDLUpdateWindow(window, renderer, &GlobalBackbuffer);
                } break;
            }
        } break;
    }

    return(shouldquit);
}
