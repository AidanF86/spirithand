#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
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
double secselapsed;

typedef struct
Debuginfo {
    bool colliders;
    SDL_Color collidercolor;
    bool worldgrid;
    SDL_Color worldgridcolor;
    double worldgridgapsize;
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
UIDocker {
    int x;
    int y;
    int totaloffset;
    bool vertical;
    int offsetscalar;
} UIDocker;

#define MAX_ANIM_FRAMES 10
typedef struct
Animation {
    SDL_Texture *frames[MAX_ANIM_FRAMES];
    int numframes;
    int currentframe;
    double frametime;
    double timetillnext;
} Animation;

typedef struct
Spirit {
    double x; double y;
    double velx; double vely;
    double size;
    SDL_Color color;
    Animation anim;
} Spirit;

int mousex;
int mousey;
bool mouseleftdown;
bool mouseleftup;

int fps;

global_variable Debuginfo debug;
global_variable Keysdown keysdown;
global_variable Camera maincam;

TTF_Font *debugfont;
#define FONTASPECTRATIO 0.5
bool debugMenuEnabled;

#define MAXSPIRITS 50 // idk 54 kinda sounds and feels right
global_variable int spiritcount;
global_variable Spirit spirits[MAXSPIRITS];
global_variable SDL_Texture *spirittextures[2];

bool **mainmap;
int mapw;
int maph;
double mapsquaresize;

void advanceanimation(Animation *anim, double time)
{
    if(anim->numframes == 0)
    {
        return;
    }
    anim->timetillnext -= time;
    if(anim->timetillnext <= 0)
    {
        anim->timetillnext = anim->frametime;
        anim->currentframe++;
        if(anim->currentframe >= anim->numframes)
        {
            anim->currentframe = 0;
        }
    }
}

SDL_Texture *
texturefromimage(SDL_Renderer *renderer, char *filename)
{
    SDL_Surface *surface = IMG_Load(filename);
    if(surface == NULL)
    {
        printf("Cannot find image file %s\n", filename);
        //SDL_Quit(); // TODO(aidan): should we really quit?
        return NULL;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
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
    newspirit.size = size;
    newspirit.color = color;
    spirits[spiritcount] = newspirit;
    return &(spirits[spiritcount++]);
}

int
updatespirits()
{
    int velchanges [spiritcount][2];
    for(int i = 0; i < spiritcount; i++)
    {
        velchanges[i][0] = 1;
        velchanges[i][1] = 1;
    }

    for(int i = 0; i < spiritcount; i++)
    {
        Spirit spirit = spirits[i];
        // handle collisions
        if(spirit.x + spirit.size / 2 + spirit.velx > maincam.x + maincam.w / 2 ||
           spirit.x - spirit.size / 2 + spirit.velx < maincam.x - maincam.w / 2)
        {
            velchanges[i][0] = -1;
        }
        if(spirit.y + spirit.size / 2 + spirit.vely > maincam.y + maincam.h / 2 ||
           spirit.y - spirit.size / 2 + spirit.vely < maincam.y - maincam.h / 2)
        {
            velchanges[i][1] = -1;
        }

        for(int a = 0; a < spiritcount; a++)
        {
            Spirit spirit2 = spirits[a];
            if(i == a)
            {
                continue;
            }

            if(willcollidex(spirit.x, spirit.y, spirit.size, spirit.size, spirit.velx,
                            spirit2.x, spirit2.y, spirit2.size, spirit2.size))
            {
                velchanges[i][0] = -1;
            }
            if(willcollidey(spirit.x, spirit.y, spirit.size, spirit.size, spirit.vely,
                            spirit2.x, spirit2.y, spirit2.size, spirit2.size))
            {
                velchanges[i][1] = -1;
            }
        }

        for(int x = 0; x < mapw; x++)
        {
            for(int y = 0; y < maph; y++)
            {
                double obstaclex = x*mapsquaresize - mapsquaresize/2.0;
                double obstacley = y*mapsquaresize - mapsquaresize/2.0;
                if(mainmap[x][y])
                {
                    if(willcollidex(spirit.x, spirit.y, spirit.size, spirit.size, spirit.velx,
                                    obstaclex, obstacley, mapsquaresize, mapsquaresize))
                    {
                        velchanges[i][0] = -1;
                    }
                    if(willcollidey(spirit.x, spirit.y, spirit.size, spirit.size, spirit.vely,
                                    obstaclex, obstacley, mapsquaresize, mapsquaresize))
                    {
                        velchanges[i][1] = -1;
                    }
                }
            }
        }

    }

    for(int i = 0; i < spiritcount; i++)
    {
        Spirit *spirit = &(spirits[i]);
        spirit->velx *= velchanges[i][0];
        spirit->vely *= velchanges[i][1];

        spirit->x += spirit->velx;
        spirit->y += spirit->vely;
    }
}

void
loadfonts()
{
    TTF_Init();
    debugfont = TTF_OpenFont("resources/fonts/JetBrainsMono-Medium.ttf", 90);
    if(debugfont == NULL)
        printf("Font could not be loaded!\n");
}

    // returns the size
int
drawtext(Camera cam, char *text, int x, int y, int h, TTF_Font *font, bool ui, UIDocker *docker)
{
    if(docker != NULL)
    {
        x = docker->x;
        y = docker->y;
        if(docker->vertical)
            y += docker->totaloffset * docker->offsetscalar;
        else
            x += docker->totaloffset * docker->offsetscalar;
    }

    if(docker != NULL)
    {
        if(docker->vertical)
            docker->totaloffset += h;
        else
            docker->totaloffset += strlen(text) * h * FONTASPECTRATIO;
    }

    int textlength = strlen(text);
    SDL_Color color;
    SDL_GetRenderDrawColor(cam.renderer, &(color.r), &(color.g), &(color.b), &(color.a));

    if(ui)
    {
        //printf("drawtext:ui (%d, %d, %d)", x, y, h);
        SDL_Rect screenrect = recttoscreen(cam, x, y, 0, h, ui);
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
        //    rect = rectuitoscreen(cam, rect.x, rect.y, rect.w, rect.h);
        //else
        //    rect = rectgametoscreen(cam, rect.x, rect.y, rect.w, rect.h);

        // TODO(aidan): definitely inefficient, should use glyph caching
        SDL_Surface *glyphcache = TTF_RenderGlyph_Solid(font, text[i], color);
        SDL_Texture *glyph = SDL_CreateTextureFromSurface(cam.renderer, glyphcache);
        SDL_RenderCopy(cam.renderer, glyph, NULL, &rect);
        SDL_FreeSurface(glyphcache);
        SDL_DestroyTexture(glyph);
    }

}

bool
drawbutton(Camera cam, UIDocker *docker, char *text, double h)
{
    bool beinghovered = false;
    bool beingheld = false;
    bool beingreleased = false;
    double x = docker->x;
    double y = docker->y;
    double w = strlen(text) * h * FONTASPECTRATIO;
    if(docker != NULL)
    {
        if(docker->vertical)
            y += docker->totaloffset * docker->offsetscalar;
        else
            x += docker->totaloffset * docker->offsetscalar;
    }

    SDL_Rect rectscreen = recttoscreen(cam, x, y, w, h, ui);
    if(!(mousex < rectscreen.x ||
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
    drawtext(cam, text, rectscreen.x, rectscreen.y, rectscreen.h, debugfont, false, NULL);
    // render border rect
    SDL_RenderDrawRect(cam.renderer, &rectscreen);

    if(docker != NULL)
    {
        if(docker->vertical)
            docker->totaloffset += h;
        else
            docker->totaloffset += w;
    }

    return beingreleased;
}

bool
drawbuttoncheckbox(Camera cam, UIDocker *docker, char *text, double h, bool checked)
{
    bool beinghovered = false;
    bool beingheld = false;
    bool beingreleased = false;
    double x = docker->x;
    double y = docker->y;
    double w = strlen(text) * h * FONTASPECTRATIO + h;

    if(docker->vertical)
        y += docker->totaloffset * docker->offsetscalar;
    else
        x += docker->totaloffset * docker->offsetscalar;


    SDL_Rect rectscreen = recttoscreen(cam, x, y, w, h, ui);
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
    //render check rect
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
    drawtext(cam, text, rectscreen.x + rectscreen.h, rectscreen.y, rectscreen.h, debugfont, false, NULL);
    // render border rect
    SDL_RenderDrawRect(cam.renderer, &rectscreen);

    if(docker->vertical)
        docker->totaloffset += h;
    else
        docker->totaloffset += w;

    return beingreleased;
}

int
drawrect(Camera cam, double x, double y, double w, double h)
{
    SDL_Rect rect = recttoscreen(cam, x, y, w, h, game);
    SDL_RenderDrawRect(cam.renderer, &rect);
}

int
drawmap(Camera cam, bool** map, int w, int h)
{
    SDL_SetRenderDrawColor(cam.renderer, 255, 255, 255, 155);
    for(int x = 0; x < w; x++)
    {
        for(int y = 0; y < h; y++)
        {
            if(map[x][y])
            {
                SDL_Rect rect = recttoscreen(cam,
                                             x*mapsquaresize - mapsquaresize/2.0,
                                             y*mapsquaresize - mapsquaresize/2.0,
                                             mapsquaresize,
                                             mapsquaresize,
                                             game);

                SDL_RenderFillRect(cam.renderer,
                                   &rect);
            }
        }
    }
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

int
drawworldgrid(Camera cam, double gapsize, SDL_Color color, int reccount)
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
    //SDL_Rect canvasrect = rectgametoscreen(cam,
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

    Spirit *spirit;
    for(int i = 0; i < spiritcount; i++)
    {
        spirit = &(spirits[i]);
        SDL_Rect spiritrect = recttoscreen(cam,
                                          spirit->x - spirit->size / 2,
                                          spirit->y - spirit->size / 2,
                                          spirit->size,
                                          spirit->size,
                                          game);

        advanceanimation(&(spirit->anim), secselapsed);
        SDL_RenderCopy(cam.renderer,
                       spirit->anim.frames[spirit->anim.currentframe],
                       NULL,
                       &spiritrect);

        // draw colliders if debug option enabled
        if(debug.colliders)
        {
            SDL_SetRenderDrawColor(cam.renderer,
                                   spirit->color.r,
                                   spirit->color.g,
                                   spirit->color.b,
                                   spirit->color.a);
            drawrect(cam,
                    spirit->x - spirit->size / 2,
                    spirit->y - spirit->size / 2,
                    spirit->size,
                    spirit->size);
        }
    }

    drawmap(cam, mainmap, mapw, maph);
    
    // render ui
    if(debugMenuEnabled)
    {
        UIDocker debugdocker;
        debugdocker.x = cam.wui*-1/2;
        debugdocker.y = cam.hui*-1/2;
        debugdocker.totaloffset = 0;
        debugdocker.vertical = false;
        debugdocker.offsetscalar = 1;
        if(drawbuttoncheckbox(cam, &debugdocker, "grid", 5, debug.worldgrid))
        {
            debug.worldgrid = !debug.worldgrid;
        }
        if(drawbuttoncheckbox(cam, &debugdocker, "colliders", 5, debug.colliders))
        {
            debug.colliders = !debug.colliders;
        }


        int debugtexth = 5;
        SDL_SetRenderDrawColor(cam.renderer, 255, 255, 255, 150);
        debugdocker.x = cam.wui*-1/2;
        debugdocker.y = cam.hui/2 - debugtexth;
        debugdocker.totaloffset = 0;
        debugdocker.vertical = true;
        debugdocker.offsetscalar = -1;

        char caminfobuffer[80];
        sprintf(caminfobuffer, "(%d, %d) %d", (int)cam.x, (int)cam.y, (int)cam.w);
        drawtext(cam, caminfobuffer, 0, 0, debugtexth, debugfont, true, &debugdocker);

        char fpstextbuffer[10];
        sprintf(fpstextbuffer, "fps: %d", fps);
        drawtext(cam, fpstextbuffer, 0, 0, debugtexth, debugfont, true, &debugdocker);
    }


    drawsidebars(cam);
    SDL_RenderPresent(cam.renderer);
}

int
updateplayermovement()
{
    double movespeed = 0.008;
    double zoomspeed = 0.008;
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

int
SDLGetWindowRefreshRate(SDL_Window *window)
{
    SDL_DisplayMode mode;
    int displayindex = SDL_GetWindowDisplayIndex(window);
    int defaultrefreshrate = 60;
    if(SDL_GetDesktopDisplayMode(displayindex, &mode) != 0 || mode.refresh_rate == 0)
    {
        return defaultrefreshrate;
    }
    return mode.refresh_rate;
}

float
SDLGetSecondsElapsed(int oldcounter, int currentcounter)
{
    return ((float)(currentcounter - oldcounter) / (float)(SDL_GetPerformanceFrequency()));
}


int main()
{
    srand(time(0));

    initcolors();
    loadfonts();

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
    debugMenuEnabled = true;

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

    mousex = maincam.x + maincam.w / 2;
    mousey = maincam.y + maincam.h / 2;

    mapw = 10;
    maph = 10;
    mapsquaresize = 10;
    mainmap = (bool**)malloc(mapw*sizeof(bool*));
    for(int i = 0; i < mapw; i++)
    {
        mainmap[i] = (bool*)malloc(maph*sizeof(bool));
        for(int a = 0; a < maph; a++)
        {
            mainmap[i][a] = randint(0, 5) == 0;
        }
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

            // load textures
            spirittextures[0] = texturefromimage(renderer, "resources/graphics/spirit0.png");
            spirittextures[1] = texturefromimage(renderer, "resources/graphics/spirit1.png");

            for(int i = 0; i < 10; i++)
            {
                Spirit *newspirit = makespirit(randdouble(-40, 40), randdouble(-40, 40), 5, color_orange);
                newspirit->anim.frames[0] = spirittextures[0];
                newspirit->anim.frames[1] = spirittextures[1];
                newspirit->anim.numframes = 2;
                newspirit->anim.currentframe = randint(0, newspirit->anim.numframes);
                newspirit->anim.frametime = 0.5;
                newspirit->anim.timetillnext = randdouble(0, newspirit->anim.frametime);


                int left = randint(0, 2);
                int up = randint(0, 2);
                double speed = 0.3;
                if(left == 0)
                    newspirit->velx = -speed;
                else
                    newspirit->velx = speed;
                if(up == 0)
                    newspirit->vely = -speed;
                else
                    newspirit->vely = speed;
            }

            int gameupdatehz = 60;
            float targetsecondsperframe = 1.0f / (float)gameupdatehz;
            int lastcounter = SDL_GetPerformanceCounter();
            secselapsed = 0;
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

                int perfcountfrequency = SDL_GetPerformanceFrequency();

                while(SDLGetSecondsElapsed(lastcounter, SDL_GetPerformanceCounter()) < targetsecondsperframe)
                {
                    int timetosleep = ((targetsecondsperframe - 
                                        SDLGetSecondsElapsed(lastcounter, SDL_GetPerformanceCounter())) * 1000);
                    if(timetosleep > 0)
                    {
                        SDL_Delay(timetosleep);
                    }
                    while (SDLGetSecondsElapsed(lastcounter, SDL_GetPerformanceCounter()) < targetsecondsperframe)
                    {
                        // waiting...
                    }
                }

                int endcounter = SDL_GetPerformanceCounter();
                int counterelapsed = endcounter - lastcounter;

                secselapsed = counterelapsed / 1000000000.0;

                double msperframe = ((1000.0f * (double)counterelapsed) / (double)perfcountfrequency);
                fps = round( (double)perfcountfrequency / (double)counterelapsed );
                lastcounter = endcounter;
            }
        }
    }

    // free memory
    for(int i = 0; i < mapw; i++)
    {
        free(mainmap[i]);
    }
    free(mainmap);

    return 0;
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
