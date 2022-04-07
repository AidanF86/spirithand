#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
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
    bool colliders;
    SDL_Color collidercolor;
    bool worldgrid;
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

typedef struct
ButtonDocker {
    int x;
    int y;
    int totaloffset;
    bool vertical;
} ButtonDocker;

int mousex;
int mousey;
bool mouseleftdown;
bool mouseleftup;

global_variable Debuginfo debug;
global_variable Keysdown keysdown;
global_variable Camera maincam;

TTF_Font *debugfont;
#define FONTASPECTRATIO 0.5

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
        // handle collisions
        if(spirit->x + spirit->size / 2 > maincam.x + maincam.w / 2 ||
           spirit->x - spirit->size / 2 < maincam.x - maincam.w / 2)
        {
            spirit->velx *= -1;
        }
        if(spirit->y + spirit->size / 2 > maincam.y + maincam.h / 2 ||
           spirit->y - spirit->size / 2 < maincam.y - maincam.h / 2)
        {
            spirit->vely *= -1;
        }

        spirit->x += spirit->velx;
        spirit->y += spirit->vely;
    }
}

void
loadfonts()
{
    TTF_Init();
    debugfont = TTF_OpenFont("resources/fonts/JetBrainsMono-Medium.ttf", 20);
    if(debugfont == NULL)
        printf("Font could not be loaded!\n");
}

    // returns the size
int
drawtext(Camera cam, char *text, int x, int y, int h, TTF_Font *font, bool ui)
{
    int textlength = strlen(text);
    SDL_Color color;
    SDL_GetRenderDrawColor(cam.renderer, &(color.r), &(color.g), &(color.b), &(color.a));

    if(ui)
    {
        //printf("drawtext:ui (%d, %d, %d)", x, y, h);
        SDL_Rect screenrect = boxtoscreen(cam, x, y, 0, h, ui);
        x = screenrect.x;
        y = screenrect.y;
        h = screenrect.h;
        //printf(" -> (%d, %d, %d)\n", x, y, h);
    }

    for(int i = 0; i < textlength; i++) {
        SDL_Rect rect;
        rect.x = x + (i * (h * FONTASPECTRATIO) ); // TODO(aidan): add spacing
        rect.y = y;
        rect.w = (h * FONTASPECTRATIO);
        rect.h = h;
        //if(ui)
        //    rect = boxuitoscreen(cam, rect.x, rect.y, rect.w, rect.h);
        //else
        //    rect = boxgametoscreen(cam, rect.x, rect.y, rect.w, rect.h);

        // TODO(aidan): definitely inefficient, should use glyph caching
        SDL_Surface *glyphcache = TTF_RenderGlyph_Solid(font, text[i], color);
        SDL_Texture *glyph = SDL_CreateTextureFromSurface(cam.renderer, glyphcache);
        SDL_RenderCopy(cam.renderer, glyph, NULL, &rect);
        SDL_FreeSurface(glyphcache);
        SDL_DestroyTexture(glyph);
    }
}

bool
drawbutton(Camera cam, ButtonDocker *docker, char *text, double h)
{
    bool beinghovered = false;
    bool beingheld = false;
    bool beingreleased = false;
    double x = docker->x;
    double y = docker->y;
    double w = strlen(text) * h * FONTASPECTRATIO;
    if(docker->vertical)
        y += docker->totaloffset;
    else
        x += docker->totaloffset;

    SDL_Rect rectscreen = boxtoscreen(cam, x, y, w, h, ui);
    if(! (mousex < rectscreen.x ||
          mousex > rectscreen.x + rectscreen.w ||
          mousey < rectscreen.y ||
          mousey > rectscreen.y + rectscreen.h))
    {
        beinghovered = true;
        if(mouseleftdown)
        {
            beingheld = true;
        }
        // TODO(aidan): else if?
        if(mouseleftup)
        {
            beingreleased = true;
        }
    }

    // render background rect
    if(beingheld)
        SDL_SetRenderDrawColor(cam.renderer, 255, 255, 255, 150);
    else if(beinghovered)
        SDL_SetRenderDrawColor(cam.renderer, 100, 100, 100, 150);
    else
        SDL_SetRenderDrawColor(cam.renderer, 55, 55, 55, 150);
    SDL_RenderFillRect(cam.renderer, &rectscreen);
    // render text
    if(beingheld)
        SDL_SetRenderDrawColor(cam.renderer, 0, 0, 0, 150);
    else
        SDL_SetRenderDrawColor(cam.renderer, 255, 255, 255, 150);
    drawtext(cam, text, rectscreen.x, rectscreen.y, rectscreen.h, debugfont, false);
    // render border rect
    SDL_RenderDrawRect(cam.renderer, &rectscreen);

    if(docker->vertical)
        docker->totaloffset += h;
    else
        docker->totaloffset += w;

    return beingreleased;
}

bool
drawbuttoncheckbox(Camera cam, ButtonDocker *docker, char *text, double h, bool checked)
{
    bool beinghovered = false;
    bool beingheld = false;
    bool beingreleased = false;
    double x = docker->x;
    double y = docker->y;
    double w = strlen(text) * h * FONTASPECTRATIO + h;
    if(docker->vertical)
        y += docker->totaloffset;
    else
        x += docker->totaloffset;

    SDL_Rect rectscreen = boxtoscreen(cam, x, y, w, h, ui);
    if(! (mousex < rectscreen.x ||
          mousex > rectscreen.x + rectscreen.w ||
          mousey < rectscreen.y ||
          mousey > rectscreen.y + rectscreen.h))
    {
        beinghovered = true;
        if(mouseleftdown)
        {
            beingheld = true;
        }
        // TODO(aidan): else if?
        if(mouseleftup)
        {
            beingreleased = true;
        }
    }

    // render background rect
    if(beingheld)
        SDL_SetRenderDrawColor(cam.renderer, 255, 255, 255, 150);
    else if(beinghovered)
        SDL_SetRenderDrawColor(cam.renderer, 100, 100, 100, 150);
    else
        SDL_SetRenderDrawColor(cam.renderer, 55, 55, 55, 150);
    SDL_RenderFillRect(cam.renderer, &rectscreen);
    // render text
    if(beingheld)
        SDL_SetRenderDrawColor(cam.renderer, 0, 0, 0, 150);
    else
        SDL_SetRenderDrawColor(cam.renderer, 255, 255, 255, 150);
    //render check box
    if(checked)
    {
        SDL_SetRenderDrawColor(cam.renderer, 255, 255, 255, 150);
        SDL_Rect checkboxrect = rectscreen;
        checkboxrect.w = rectscreen.h;
        checkboxrect.x += rectscreen.h / 4;
        checkboxrect.y += rectscreen.h / 4;
        checkboxrect.w -= rectscreen.h / 4 * 2;
        checkboxrect.h -= rectscreen.h / 4 * 2;
        SDL_RenderFillRect(cam.renderer, &checkboxrect);
    }
    drawtext(cam, text, rectscreen.x + rectscreen.h, rectscreen.y, rectscreen.h, debugfont, false);
    // render border rect
    SDL_RenderDrawRect(cam.renderer, &rectscreen);

    if(docker->vertical)
        docker->totaloffset += h;
    else
        docker->totaloffset += w;

    return beingreleased;
}

int
drawbox(Camera cam, double x, double y, double w, double h)
{
    SDL_Rect rect = boxtoscreen(cam, x, y, w, h, game);
    SDL_RenderDrawRect(cam.renderer, &rect);
}

int
drawworldgridaux(Camera cam, double gapsize, SDL_Color color)
{
    color.a = color.a / 4;

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
        Vectori minscreenpos = vectorfgametoscreen(cam, x, miny);
        Vectori maxscreenpos = vectorfgametoscreen(cam, x, maxy);
        SDL_RenderDrawLine(cam.renderer,
                           minscreenpos.x,
                           minscreenpos.y,
                           maxscreenpos.x,
                           maxscreenpos.y);
    }
    for(int y = miny; y <= maxy; y+= gapsize)
    {
        Vectori minscreenpos = vectorfgametoscreen(cam, minx, y);
        Vectori maxscreenpos = vectorfgametoscreen(cam, maxx, y);
        SDL_RenderDrawLine(cam.renderer,
                           minscreenpos.x,
                           minscreenpos.y,
                           maxscreenpos.x,
                           maxscreenpos.y);
    }
}

int drawworldgrid(Camera cam, double gapsize, SDL_Color color, int reccount)
{
    if(reccount >= 7)
    {
        printf("drawworldgrid: WTF\n");
        return 1;
    }

    // find appropriate gap size
    if(cam.w / gapsize > 8.0 || cam.h / gapsize > 8.0) 
    {
        //printf("Nested drawgrid: size=%f\n", gapsize);
        drawworldgrid(cam, gapsize * 2.0, color, reccount + 1);
        return 0;
    }
    else if (cam.w / gapsize < 4.0 || cam.h / gapsize < 4.0) 
    {
        //printf("Nested drawgrid: size=%f\n", gapsize);
        drawworldgrid(cam, gapsize / 2.0, color, reccount + 1);
        return 0;
    }

    double subalpha = ((cam.w / gapsize) - 4.0) / 4.0;
    subalpha = (1.0 - subalpha) * 255;

    // render sub-grid
    SDL_Color subcolor = color;
    subcolor.a = subalpha;
    drawworldgridaux(cam, gapsize / 2.0, subcolor);

    drawworldgridaux(cam, gapsize, color);
}

int
render(Camera cam)
{
    SDL_SetRenderDrawColor(cam.renderer, 155, 155, 155, 255);
    SDL_RenderClear(cam.renderer);
        // draw background canvas (to make sure of the size)
    //SDL_Rect canvasrect = boxgametoscreen(cam,
    //                                    -cam.w / 2 + cam.x,
    //                                    -cam.h / 2 + cam.y,
    //                                    cam.w,
    //                                    cam.h);
    //SDL_SetRenderDrawColor(cam.renderer, 155, 155, 155, 255);
    //SDL_RenderFillRect(cam.renderer, &canvasrect);

    if(debug.worldgrid)
    {
        drawworldgrid(cam, debug.worldgridgapsize, debug.worldgridcolor, 0);
    }

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
            drawbox(cam,
                           spirit.x - spirit.size / 2,
                           spirit.y - spirit.size / 2,
                           spirit.size,
                           spirit.size
                           );
            SDL_SetRenderDrawColor(cam.renderer, 255, 0, 0, 255);
            drawbox(cam, -10, -10, 20, 20);
        }
    }
    
    // render ui
    ButtonDocker debugdocker;
    debugdocker.x = cam.wui*-1/2;
    debugdocker.y = cam.hui*-1/2;
    debugdocker.totaloffset = 0;
    debugdocker.vertical = false;
    if(drawbuttoncheckbox(cam, &debugdocker, "grid", 5, debug.worldgrid))
    {
        debug.worldgrid = !debug.worldgrid;
    }
    if(drawbuttoncheckbox(cam, &debugdocker, "colliders", 5, debug.colliders))
    {
        debug.colliders = !debug.colliders;
    }

    char caminfobuffer[80];
    sprintf(caminfobuffer, "(%d, %d) %d", (int)cam.x, (int)cam.y, (int)cam.w);
    int caminfoh = 5;
    drawtext(cam, caminfobuffer, cam.wui*-1/2, cam.hui/2 - caminfoh, caminfoh, debugfont, true);


    drawsidebars(cam);
    SDL_RenderPresent(cam.renderer);
}

int
updateplayermovement()
{
    double movespeed = 0.002;
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
    //printf("maincam.w=%f\n", maincam.w);
    //printf("maincam.h=%f\n", maincam.h);

    maincam.x += poschange.x * maincam.w;
    maincam.y += poschange.y * maincam.h;
}



int main()
{
    srand(time(0));

    initcolors();
    loadfonts();

    debug.fps = 0;
    debug.collidercolor.r = 255;
    debug.collidercolor.g = 255;
    debug.collidercolor.b = 255;
    debug.collidercolor.a = 255;
    debug.colliders = true;
    debug.worldgridcolor.r = 0;
    debug.worldgridcolor.g = 0;
    debug.worldgridcolor.b = 0;
    debug.worldgridcolor.a = 255;
    debug.worldgridgapsize = 20;
    debug.worldgrid = true;

    maincam.x = 0;
    maincam.y = 0;
    maincam.w = 100;
    maincam.h = 100;
    maincam.wui = 100;
    maincam.hui = 100;
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

    mousex = maincam.x + maincam.w / 2;
    mousey = maincam.y + maincam.h / 2;

    spiritcount = 0;
    for(int i = 0; i < 10; i++)
    {
        Spirit *newspirit = makespirit(randdouble(-40, 40), randdouble(-40, 40), 10, color_orange);
        int left = randint(0, 2);
        int up = randint(0, 2);
        double speed = 0.1;
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

                mouseleftup = false;
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
                //usleep(1000);
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
            mousex = event->motion.x;
            mousey = event->motion.y;
        } break;
        case SDL_MOUSEBUTTONDOWN:
        {
            if(event->button.button == SDL_BUTTON_LEFT)
            {
                mouseleftdown = true;
            }
        } break;
        case SDL_MOUSEBUTTONUP:
        {
            if(event->button.button == SDL_BUTTON_LEFT)
            {
                mouseleftdown = false;
                mouseleftup = true;
            }
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
