#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define internal static
#define local_persist static
#define global_variable static

typedef struct
Box {
    double x; double y;
    double w; double h;
} Box;

typedef struct
circle {
    double x; double y;
    double radius;
} circle;

typedef struct
Vectorf {
    double x; double y;
} Vectorf;

typedef struct
Vectori {
    int x; int y;
} Vectori;

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
global_variable Vectori mousepos;

global_variable Box testcollider;
global_variable Box playercollider;


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
    {
        return true;
    }
    return false;
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
boxtocamera(Camera cam, double x, double y, double w, double h)
{
    SDL_Rect rectincam;
    rectincam.x = (x - cam.x) / cam.w * cam.wres;
    rectincam.y = (y - cam.y) / cam.h * cam.hres;
    rectincam.w = w / cam.w * cam.wres;
    rectincam.h = h / cam.h * cam.hres;
    return rectincam;
}

int
debugrenderbox(Camera cam, double x, double y, double w, double h)
{
    SDL_Rect rect = boxtocamera(cam, x, y, w, h);
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
    debugrenderbox(cam,
                   testcollider.x,
                   testcollider.y,
                   testcollider.w,
                   testcollider.h);
    debugrenderbox(cam,
                   playercollider.x,
                   playercollider.y,
                   playercollider.w,
                   playercollider.h);
    if(boxescolliding(testcollider.x,
                      testcollider.y,
                      testcollider.w,
                      testcollider.h,
                      playercollider.x,
                      playercollider.y,
                      playercollider.w,
                      playercollider.h))
    {
        printf("Boxes colliding!\n");
    }
    SDL_RenderPresent(cam.renderer);
    SDL_RenderPresent(cam.renderer);
}

int
updatemovement()
{
    float speed = 0.01;
    if(keysdown.up)
    {
        maincam.y -= speed;
    }
    if(keysdown.down)
    {
        maincam.y += speed;
    }
    if(keysdown.left)
    {
        maincam.x -= speed;
    }
    if(keysdown.right)
    {
        maincam.x += speed;
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
            playercollider.x = mousepos.x - playercollider.w / 2;
            playercollider.y = mousepos.y - playercollider.h / 2;
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
    debug.fps = 0;
    debug.collidercolor.r = 255;
    debug.collidercolor.g = 255;
    debug.collidercolor.b = 255;
    debug.collidercolor.a = 255;
    debug.colliders = 0;

    maincam.x = 0;
    maincam.y = 0;
    maincam.w = 100;
    maincam.h = 100;
    maincam.wres = 640;
    maincam.hres = 480;

    mousepos.x = maincam.x + maincam.w / 2;
    mousepos.y = maincam.y + maincam.h / 2;

    testcollider.x = 30;
    testcollider.y = 25;
    testcollider.w = 20;
    testcollider.h = 15;

    playercollider.x = 50;
    playercollider.y = 50;
    playercollider.w = 10;
    playercollider.h = 10;


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
                render(maincam);
                sleep(0.01);
            }
        }
    }

}
