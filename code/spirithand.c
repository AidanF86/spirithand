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
    double worldgridsize;
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

#define MAXSPIRITS 54 // idk 54 kinda sounds and feels right
global_variable int spiritcount;
global_variable Spirit spirits[MAXSPIRITS];


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

        // do collision checking
        double potentialx = spirit->x + spirit->velx;
        double potentialy = spirit->y + spirit->vely;
        if(!boxtotallyinbox(potentialx, potentialy, spirit->box.w, spirit->box.h,
                            maincam.x, maincam.y, maincam.w, maincam.h))
        {
            double incpercent;
            double incx;
            double incy;
            if(potentialx / maincam.x > potentialy / maincam.y)
            {
                // do horizontal
                incx = maincam.x - spirit->x;
                incpercent = incx / spirit->velx;
                incy = incpercent * spirit->vely;
                spirit->x += incx;
                spirit->y += incy;
                Vectorf newvel = bounce(spirit->velx, spirit->vely, false);
                spirit->velx = newvel.x;
                spirit->vely = newvel.y;

                potentialx = spirit->x + spirit->velx * (1-incpercent);
                potentialy = spirit->y + spirit->vely * (1-incpercent);

                if(potentialy / maincam.y > potentialx / maincam.x)
                {
                    // do vertical
                    incy = maincam.y - spirit->y;
                    incpercent += incy / spirit->vely;
                    incx = incpercent * spirit->velx;
                    spirit->x += incx;
                    spirit->y += incy;
                    Vectorf newvel = bounce(spirit->velx, spirit->vely, false);
                    spirit->velx = newvel.x;
                    spirit->vely = newvel.y;
                }
                spirit->x += (1-incpercent) * spirit->velx;
                spirit->y += (1-incpercent) * spirit->vely;
            }
            if(potentialy / maincam.y > potentialx / maincam.x)
            {
                // do vertical
                incy = maincam.y - spirit->y;
                incpercent += incy / spirit->vely;
                incx = incpercent * spirit->velx;
                spirit->x += incx;
                spirit->y += incy;
                Vectorf newvel = bounce(spirit->velx, spirit->vely, false);
                spirit->velx = newvel.x;
                spirit->vely = newvel.y;

                potentialx = spirit->x + spirit->velx * (1-incpercent);
                potentialy = spirit->y + spirit->vely * (1-incpercent);

                if(potentialx / maincam.x > potentialy / maincam.y)
                {
                    // do horizontal
                    incx = maincam.x - spirit->x;
                    incpercent = incx / spirit->velx;
                    incy = incpercent * spirit->vely;
                    spirit->x += incx;
                    spirit->y += incy;
                    Vectorf newvel = bounce(spirit->velx, spirit->vely, false);
                    spirit->velx = newvel.x;
                    spirit->vely = newvel.y;
                }
                spirit->x += (1-incpercent) * spirit->velx;
                spirit->y += (1-incpercent) * spirit->vely;
            }
        }


        spirit->box.x = spirit->x - spirit->box.w / 2.0;
        spirit->box.y = spirit->y - spirit->box.h / 2.0;
    }
}

int
debugrenderbox(Camera cam, Box box)
{
    SDL_Rect rect = boxtoscreen(cam, box.x, box.y, box.w, box.h);
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
    double minx = cam.x - cam.w / 2.0 - fmod(cam.x - cam.w / 2.0, debug.worldgridsize);
    double miny = cam.y - cam.h / 2.0 - fmod(cam.y - cam.h / 2.0, debug.worldgridsize);
    double maxx = cam.x + cam.w / 2.0 + fmod(cam.x + cam.w / 2.0, debug.worldgridsize);
    double maxy = cam.y + cam.h / 2.0 + fmod(cam.y + cam.h / 2.0, debug.worldgridsize);
    minx -= debug.worldgridsize;
    miny -= debug.worldgridsize;
    maxx += debug.worldgridsize;
    maxy += debug.worldgridsize;

    SDL_SetRenderDrawColor(cam.renderer,
                           debug.worldgridcolor.r,
                           debug.worldgridcolor.g,
                           debug.worldgridcolor.b,
                           debug.worldgridcolor.a);
    for(int x = minx; x <= maxx; x+= debug.worldgridsize)
    {
        Vectori minscreenpos = vectorftoscreen(cam, x, miny);
        Vectori maxscreenpos = vectorftoscreen(cam, x, maxy);
        SDL_RenderDrawLine(cam.renderer,
                           minscreenpos.x,
                           minscreenpos.y,
                           maxscreenpos.x,
                           maxscreenpos.y);
    }
    for(int y = miny; y <= maxy; y+= debug.worldgridsize)
    {
        Vectori minscreenpos = vectorftoscreen(cam, minx, y);
        Vectori maxscreenpos = vectorftoscreen(cam, maxx, y);
        SDL_RenderDrawLine(cam.renderer,
                           minscreenpos.x,
                           minscreenpos.y,
                           maxscreenpos.x,
                           maxscreenpos.y);
    }
}

int
render(Camera cam)
{
    SDL_SetRenderDrawColor(cam.renderer, 0, 0, 0, 255);
    SDL_RenderClear(cam.renderer);
        // draw background canvas (to make sure of the size)
    SDL_Rect canvasrect = boxtoscreen(cam,
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
    double speed = 0.1;
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
    debug.worldgridcolor.r = 0;
    debug.worldgridcolor.g = 0;
    debug.worldgridcolor.b = 0;
    debug.worldgridcolor.a = 255;
    debug.worldgridsize = 20;

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
        int left = randint(0, 2);
        int up = randint(0, 2);
        double speed = 0.05;
        if(left == 0)
            newspirit->velx = -speed;
        else
            newspirit->velx = speed;
        if(up == 0)
            newspirit->vely = -speed;
        else
            newspirit->vely = speed;

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
                usleep(1660);
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
