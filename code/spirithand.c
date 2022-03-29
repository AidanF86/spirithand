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
    double size;
    SDL_Color color;
} Spirit;

typedef struct
Debuginfo {
    int colliders;
    SDL_Color collidercolor;
    int worldgrid;
    SDL_Color worldgridcolor;
    double worldgridgapsize;
    int fps;
} Debuginfo;

typedef struct
Keysdown {
    bool up;
    bool down;
    bool left;
    bool right;
    bool z;
    bool x;
} Keysdown;

global_variable Debuginfo debug;
global_variable Keysdown keysdown;
global_variable Camera maincam;
global_variable Vectorf mousepos;

#define MAXSPIRITS 50 // idk 54 kinda sounds and feels right
global_variable int spiritcount;
global_variable Spirit spirits[MAXSPIRITS];

#define MAXOBSTACLES 50
global_variable int maxobstacles;
global_variable Box obstacles[MAXOBSTACLES];

Box bounds[4];

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
    newspirit.size = size;
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
    }
}

int
debugrenderbox(Camera cam, double x, double y, double w, double h)
{
    SDL_Rect rect = boxtoscreen(cam, x, y, w, h);
    /*
    SDL_SetRenderDrawColor(cam.renderer,
                           debug.collidercolor.r,
                           debug.collidercolor.g,
                           debug.collidercolor.b,
                           debug.collidercolor.a);
                           */
    SDL_RenderDrawRect(cam.renderer, &rect);
}

int debugrenderworldgrid(Camera cam, double gapsize, SDL_Color color, int reccount)
{
    if(reccount >= 7)
    {
        printf("debugrenderworldgrid: WTF\n");
        return 1;
    }

    // find appropriate gap size
    if(cam.w / gapsize > 8.0 || cam.h / gapsize > 8.0) 
    {
        //printf("Nested debugrendergrid: size=%f\n", gapsize);
        debugrenderworldgrid(cam, gapsize * 2.0, color, reccount + 1);
        return 0;
    }
    else if (cam.w / gapsize < 4.0 || cam.h / gapsize < 4.0) 
    {
        //printf("Nested debugrendergrid: size=%f\n", gapsize);
        debugrenderworldgrid(cam, gapsize / 2.0, color, reccount + 1);
        return 0;
    }

    // render sub-grid
    SDL_Color subcolor = color;
    subcolor.a /= 5;
    debugrenderworldgridaux(cam, gapsize / 2.0, subcolor);

    debugrenderworldgridaux(cam, gapsize, color);
}

int
debugrenderworldgridaux(Camera cam, double gapsize, SDL_Color color)
{

    double minx = cam.x - cam.w / 2.0 - fmod(cam.x - cam.w / 2.0, gapsize);
    double miny = cam.y - cam.h / 2.0 - fmod(cam.y - cam.h / 2.0, gapsize);
    double maxx = cam.x + cam.w / 2.0 + fmod(cam.x + cam.w / 2.0, gapsize);
    double maxy = cam.y + cam.h / 2.0 + fmod(cam.y + cam.h / 2.0, gapsize);
    minx -= gapsize;
    miny -= gapsize;
    maxx += gapsize;
    maxy += gapsize;

    SDL_SetRenderDrawColor(cam.renderer,
                           color.r,
                           color.g,
                           color.b,
                           color.a);
    for(int x = minx; x <= maxx; x+= gapsize)
    {
        Vectori minscreenpos = vectorftoscreen(cam, x, miny);
        Vectori maxscreenpos = vectorftoscreen(cam, x, maxy);
        SDL_RenderDrawLine(cam.renderer,
                           minscreenpos.x,
                           minscreenpos.y,
                           maxscreenpos.x,
                           maxscreenpos.y);
    }
    for(int y = miny; y <= maxy; y+= gapsize)
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

    debugrenderworldgrid(cam, debug.worldgridgapsize, debug.worldgridcolor, 0);

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
            debugrenderbox(cam,
                           spirit.x - spirit.size / 2,
                           spirit.y - spirit.size / 2,
                           spirit.size,
                           spirit.size
                           );
            SDL_SetRenderDrawColor(maincam.renderer, 255, 0, 0, 255);
            debugrenderbox(maincam, -10, -10, 20, 20);
        }
    }
    rendersidebars(cam);
    SDL_RenderPresent(cam.renderer);
}

int
updateplayermovement()
{
    double movespeed = 0.001;
    double zoomspeed = 0.002;
    Vectorf poschange = {0};
    double zoomchange = 0;
    if(keysdown.up)
    {
        poschange.y -= movespeed;
    }
    if(keysdown.down)
    {
        poschange.y += movespeed;
    }
    if(keysdown.left)
    {
        poschange.x -= movespeed;
    }
    if(keysdown.right)
    {
        poschange.x += movespeed;
    }
    if(keysdown.z)
    {
        zoomchange -= zoomspeed;
    }
    if(keysdown.x)
    {
        zoomchange += zoomspeed;
    }
        
    maincam.w += zoomchange * maincam.w;
    maincam.h += zoomchange * maincam.h;
    if(maincam.w < maincam.minw)
    {
        maincam.w = maincam.minw;
    }
    else if(maincam.w > maincam.maxw)
    {
        maincam.w = maincam.maxw;
    }
    if(maincam.h < maincam.minh)
    {
        maincam.h = maincam.minh;
    }
    else if(maincam.h > maincam.maxh)
    {
        maincam.h = maincam.maxh;
    }
    printf("maincam.w=%f\n", maincam.w);
    printf("maincam.h=%f\n", maincam.h);

    maincam.x += poschange.x * maincam.w;
    maincam.y += poschange.y * maincam.h;
    mousepos.x += poschange.x;
    mousepos.y += poschange.y;
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
    debug.worldgridgapsize = 20;

    maincam.x = 0;
    maincam.y = 0;
    maincam.w = 100;
    maincam.h = 100;
    maincam.minw = 40;
    maincam.minh = 40; // TODO(aidan): find out why values below 40
                       // cause world grid rendering bugs
    maincam.maxw = maincam.w * 50.0;
    maincam.maxh = maincam.h * 50.0;
    maincam.wres = 640;
    maincam.hres = 480;

    bounds[0].x = maincam.x - maincam.w;
    bounds[0].y = maincam.y;
    bounds[0].w = maincam.w; bounds[1].h = maincam.h;
    bounds[1].x = maincam.x;
    bounds[1].y = maincam.y - maincam.h;
    bounds[1].w = maincam.w; bounds[1].h = maincam.h;
    bounds[2].x = maincam.x + maincam.w;
    bounds[2].y = maincam.y;
    bounds[2].w = maincam.w; bounds[1].h = maincam.h;
    bounds[3].x = maincam.x;
    bounds[3].y = maincam.y + maincam.h;
    bounds[3].w = maincam.w; bounds[1].h = maincam.h;

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
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
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
                updateplayermovement();
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
            else if(keycode == SDLK_z)
            {
                keysdown.z = true;
            }
            else if(keycode == SDLK_x)
            {
                keysdown.x = true;
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
            else if(keycode == SDLK_z)
            {
                keysdown.z = false;
            }
            else if(keycode == SDLK_x)
            {
                keysdown.x = false;
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
