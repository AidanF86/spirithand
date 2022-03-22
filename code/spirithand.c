#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define internal static
#define local_persist static
#define global_variable static

#include "colors.c"

typedef struct
Vectorf {
    double x; double y;
} Vectorf;

typedef struct
Vectori {
    int x; int y;
} Vectori;

typedef struct
Box {
    double x; double y;
    double w; double h;
} Box;

typedef struct
Spirit {
    double x; double y;
    Vectorf vel;
    Box box;
    SDL_Color color;
} Spirit;

typedef struct
Camera {
    double x; double y;
    double w; double h; // in game units
    int wres; int hres;
    //double zoom;
    SDL_Renderer *renderer;
} Camera;

typedef struct
Debuginfo {
    int colliders;
    SDL_Color collidercolor;
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


    // amount should be between 0.0 and 1.0
double
interpolatelinear(double amount, double a, double b)
{
    int result = amount;
    result *= abs(a - b);
    if(a > b)
        result += b;
    else
        result += a;
    return result;
}

bool
boxescolliding(Box box1, Box box2)
{
    if(box1.x < box2.x + box2.w &&
       box1.x + box1.w > box2.x &&
       box1.y < box2.y + box2.h &&
       box1.h + box1.y > box2.y)
        return true;
    return false;
}

bool
boxtotallyinbox(Box box1, Box box2)
{
    if(box1.x < box2.x ||
       box1.x > box2.x + box2.w ||
       box1.y < box2.y ||
       box1.y > box2.y + box2.h)
        return false;
    return true;
}

Vectorf
boxdifference(double x1, double y1, double w1, double h1, double x2, double y2, double w2, double h2)
{
    Vectorf diff;
    diff.x = x1 + w1 - x2;
    diff.x = y1 + h1 - y2;
    return diff;
}

int
pointinbox(double xpt, double ypt, double xbox, double ybox, double wbox, double hbox)
{
    if(xpt < xbox ||
       xpt > xbox + wbox ||
       ypt < ybox ||
       ypt > ybox + hbox)
        return 0;
    return 1;
}

int
boxandcirclecolliding(double x1, double y1, double radius, double x2, double y2, double w, double h)
{
        // get closest point to center of box
    Vectorf boxcenter;
    boxcenter.x = x2 + w / 2;
    boxcenter.y = y2 + h / 2;

    double angle = atan( fabs(boxcenter.x - x1) / fabs(boxcenter.y - y1) );

    double pointx = sin(angle) / radius;
    double pointy = cos(angle) / radius;

    return pointinbox(pointx, pointy, x2, y2, w, h);
}

Vectori
vectorftocamera(Camera cam, double x, double y)
{
    Vectori posincam;
    posincam.x = (x - cam.x) / cam.w * cam.wres;
    posincam.y = (y - cam.y) / cam.h * cam.hres;
    return posincam;
}

Vectorf
vectorftogame(Camera cam, double x, double y)
{
    Vectorf posingame;
    posingame.x = x / cam.wres * cam.w + cam.x;
    posingame.y = y / cam.hres * cam.h + cam.y;
    return posingame;
}

SDL_Rect
boxtocamspace(Camera cam, double x, double y, double w, double h)
{
    SDL_Rect rectincam;
    rectincam.x = (x - cam.x) / cam.w * cam.wres;
    rectincam.y = (y - cam.y) / cam.h * cam.hres;
    rectincam.w = w / cam.w * cam.wres;
    rectincam.h = h / cam.h * cam.hres;
    return rectincam;
}

    // converts the camera bounds to a Box
Box
camtobox(Camera cam)
{
    Box box;
    box.x = cam.x - cam.w / 2;
    box.y = cam.y - cam.h / 2;
    box.w = cam.w;
    box.h = cam.h;
}

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
    newspirit.box.x = newspirit.x + size / 2.0;
    newspirit.box.y = newspirit.y + size / 2.0;
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
        spirit->box.x = spirit->x - spirit->box.w / 2.0;
        spirit->box.y = spirit->y - spirit->box.h / 2.0;
    }
}

int
debugrenderbox(Camera cam, Box box)
{
    SDL_Rect rect = boxtocamspace(cam, box.x, box.y, box.w, box.h);
    SDL_SetRenderDrawColor(cam.renderer,
                           debug.collidercolor.r,
                           debug.collidercolor.g,
                           debug.collidercolor.b,
                           debug.collidercolor.a);
    SDL_RenderDrawRect(cam.renderer, &rect);
}

int
render(Camera cam)
{
    SDL_SetRenderDrawColor(cam.renderer, 0, 0, 0, 255);
    SDL_RenderClear(cam.renderer);
    Spirit spirit;
    for(int i = 0; i < spiritcount; i++)
    {
        spirit = spirits[i];
        if(debug.colliders)
        {
            SDL_SetRenderDrawColor(cam.renderer,
                                   debug.collidercolor.r,
                                   debug.collidercolor.g,
                                   debug.collidercolor.b,
                                   debug.collidercolor.a);
            debugrenderbox(cam, spirit.box);
        }
    }
    SDL_RenderPresent(cam.renderer);
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
                    printf("SDL_WINDOWEVENT_RESIZED (%d, %d)\n", event->window.data1, event->window.data2);
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

int main()
{
    initcolors();

    debug.fps = 0;
    debug.collidercolor.r = 255;
    debug.collidercolor.g = 255;
    debug.collidercolor.b = 255;
    debug.collidercolor.a = 255;
    debug.colliders = 1;

    maincam.x = 0;
    maincam.y = 0;
    maincam.w = 100;
    maincam.h = 100;
    maincam.wres = 640;
    maincam.hres = 480;

    mousepos.x = maincam.x + maincam.w / 2;
    mousepos.y = maincam.y + maincam.h / 2;

    spiritcount = 0;
    makespirit(0, 0, 20, color_orange);
    makespirit(-30, -20, 26, color_blue);

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
                sleep(0.015);
            }
        }
    }

}
