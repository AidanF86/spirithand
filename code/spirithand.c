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
double deltatime;

typedef struct
DebugInfo {
    bool colliders;
    SDL_Color collidercolor;
    bool worldgrid;
    SDL_Color worldgridcolor;
    double worldgridgapsize;
    bool freecam;
} DebugInfo;

typedef struct
KeysDown {
    bool up;
    bool down;
    bool left;
    bool right;
    bool z;
    bool x;
} KeysDown;

typedef struct
KeysUp {
    bool z;
} KeysUp;

typedef struct
UIDocker {
    int x;
    int y;
    int totaloffset;
    bool vertical;
    int offsetscalar;
} UIDocker;
enum spiritstates {state_beingheld, state_thrown, state_seekinggrid, state_free};

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
Particle {
    double x, y;
    double velx, vely;
    double w, h;
    double delw, delh;
    double angle;
    double angvel;
    double alpha;
    double delalpha;
    double timeleft;
    bool diewhensmall;
    char drawpriority;
    SDL_Color color;
    Animation anim;
} Particle;

Particle spirittrailparticle;

SDL_Texture *particle1textures[1];
Animation particle1anim;

#define MAX_PARTICLES 10000
typedef struct
ParticleSystem {
    Particle particles[MAX_PARTICLES];
    int numparticles;
} ParticleSystem;

ParticleSystem mainps;

typedef struct
Spirit {
    double x; double y;
    double velx; double vely;
    double size;
    double mapx; double mapy;
    double movespeed;
    enum directions dir;
    double timebetweenparticles;
    double timetonextparticle;
    enum spiritstates state;
    SDL_Color color;
    Animation anim;
} Spirit;

double spiritdrag = 0.5;

double playerx;
double playery;
double playervelx;
double playervely;
enum directions playerdir;
double playermaxspeed = 90;
double playeracceleration = 2000;
double playerdrag = 7;
double playerw = 18 / 1.4;
double playerh = 25 / 1.4;
double spiritselectradius = 20;
Spirit *selectedspirit;
#define PLAYERANIMATIONCOUNT 4
Animation playeranims[PLAYERANIMATIONCOUNT];
// anim order: down, up, left, right
Animation *currentplayeranim;

int mousex;
int mousey;
bool mouseleftdown;
bool mouseleftup;


int fps;

global_variable DebugInfo debug;
global_variable KeysDown keysdown;
global_variable KeysUp   keysup;

global_variable Camera maincam;
double cameramovespeed = 50;
double camerazoomspeed = 1;

TTF_Font *debugfont;
#define FONTASPECTRATIO 0.5
bool debugMenuEnabled;

#define MAXSPIRITS 50
global_variable int spiritcount;
global_variable Spirit spirits[MAXSPIRITS];
global_variable SDL_Texture *spirittextures[2];

bool **mainmap;
int mapw;
int maph;
double mapsquaresize;

#include "resourceloading.c"
#include "drawing.c"



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

Particle
makeparticle(Animation anim)
{
    Particle particle = {0};
    particle.anim = anim;
}

int
addparticle(ParticleSystem *ps, Particle particle)
{
    if(ps->numparticles == MAX_PARTICLES)
    {
        printf("ERROR(addparticle): Max number of particles reached!\n");
        return 1;
    }
    ps->particles[ps->numparticles] = particle;
    ps->numparticles++;
    return 0;
}

Spirit*
makespirit(double mapx, double mapy, double size, SDL_Color color)
{
    if(spiritcount == MAXSPIRITS)
    {
        printf("makespirit: spiritcount is already at MAXSPIRITS!\n");
        return NULL;
    }
    Spirit newspirit;
    newspirit.mapx = mapx;
    newspirit.mapy = mapy;
    newspirit.x = mapx * mapsquaresize;
    newspirit.y = mapy * mapsquaresize;
    newspirit.movespeed = 0.01;
    printf("makespirit: newspirit.x: %d, newspirit.y: %d\n", (int)newspirit.x, (int)newspirit.y);
    newspirit.size = size;
    newspirit.state = state_free;
    newspirit.color = color;
    newspirit.anim.frames[0] = spirittextures[0];
    //newspirit.anim.frames[1] = spirittextures[1];
    newspirit.anim.numframes = 1;
    newspirit.anim.currentframe = randint(0, newspirit.anim.numframes);
    newspirit.anim.frametime = 0.5;
    newspirit.anim.timetillnext = randdouble(0, newspirit.anim.frametime);

    spirits[spiritcount] = newspirit;
    return &(spirits[spiritcount++]);
}

enum directions
getnewdir(bool **map, double x, double y, enum directions currentdir)
{
    int xi = roundtoint(x);
    int yi = roundtoint(y);
    enum directions possibledirs[4];
    int possibledirscount = 0;
    if(currentdir != dir_down)
    {
        if(!map[xi][yi+1])
        {
            possibledirs[possibledirscount++] = dir_down;
        }
    }
    if(currentdir != dir_up)
    {
        if(!map[xi][yi-1])
        {
            possibledirs[possibledirscount++] = dir_up;
        }
    }
    if(currentdir != dir_left)
    {
        if(!map[xi-1][yi])
        {
            possibledirs[possibledirscount++] = dir_left;
        }
    }
    if(currentdir != dir_right)
    {
        if(!map[xi+1][yi])
        {
            possibledirs[possibledirscount++] = dir_right;
        }
    }
    return possibledirs[randint(0, possibledirscount)];
}

Vectori
getnextpos(double x, double y, enum directions dir)
{
    int xi = roundtoint(x);
    int yi = roundtoint(y);
    return addvectori(newvectori(x, y), vectorofdir(dir));
}

Spirit
updatespiritposition(Spirit spirit, bool **map)
{
    if(spirit.state == state_free)
    {
        // spirits should have a chance of turning at intersections
        Vectori nextpos = getnextpos(spirit.mapx, spirit.mapy, spirit.dir);
        if(map[nextpos.x][nextpos.y])
        {
            spirit.dir = getnewdir(map, spirit.mapx, spirit.mapy, spirit.dir);
        }
        spirit.mapx += vectorofdir(spirit.dir).x * spirit.movespeed;
        spirit.mapy += vectorofdir(spirit.dir).y * spirit.movespeed;
        spirit.x = spirit.mapx * mapsquaresize;
        spirit.y = spirit.mapy * mapsquaresize;
    }
    else if(spirit.state == state_thrown)
    {
        int velchangex = 1;
        int velchangey = 1;
        for(int x = 0; x < mapw; x++)
        {
            for(int y = 0; y < maph; y++)
            {
                double obstaclex = x*mapsquaresize;
                double obstacley = y*mapsquaresize;
                if(mainmap[x][y])
                {
                    if(willcollidex(spirit.x, spirit.y, spirit.size, spirit.size, spirit.velx * deltatime,
                                    obstaclex, obstacley, mapsquaresize, mapsquaresize, 0))
                    {
                        velchangex = -1;

                    }
                    if(willcollidey(spirit.x, spirit.y, spirit.size, spirit.size, spirit.vely * deltatime,
                                    obstaclex, obstacley, mapsquaresize, mapsquaresize, 0))
                    {
                        velchangey = -1;
                    }
                }
            }
        }
        spirit.velx *= velchangex;
        spirit.vely *= velchangey;

        spirit.velx = applydrag(spirit.velx, spiritdrag);
        spirit.vely = applydrag(spirit.vely, spiritdrag);

        if(spirit.velx == 0 && spirit.vely == 0)
        {
            spirit.state = state_free;
            spirit.mapx = spirit.x / mapsquaresize;
            spirit.mapy = spirit.y / mapsquaresize;
        }

        spirit.x += spirit.velx * deltatime;
        spirit.y += spirit.vely * deltatime;
    }
    else if(spirit.state == state_beingheld)
    {
        spirit.x = playerx;
        spirit.y = playery - 12;
    }

    return spirit;
}

int
updatespirits()
{
    // update spirit positions
    for(int i = 0; i < spiritcount; i++)
    {
        spirits[i] = updatespiritposition(spirits[i], mainmap);
    }

    // update spirit selection
    bool aspiritselected = false;
    for(int i = 0; i < spiritcount; i++)
    {
        if(spirits[i].state == state_free)
        {
            if(sqrt((spirits[i].x-playerx)*(spirits[i].x-playerx) + 
                    (spirits[i].y-playery)*(spirits[i].y-playery)) <=
                    spiritselectradius)
            {
                if(selectedspirit == NULL || 
                   (selectedspirit != NULL &&
                    selectedspirit->state == state_free ||
                    selectedspirit->state == state_seekinggrid))
                {
                    selectedspirit = &(spirits[i]);
                    aspiritselected = true;
                }
            }
        }
    }
    if(!aspiritselected && selectedspirit != NULL && !selectedspirit->state == state_beingheld)
    {
        selectedspirit = NULL;
    }

    // update spirit particle emission
    for(int i = 0; i < spiritcount; i++)
    {
        Spirit *spirit = &(spirits[i]);
        spirit->timetonextparticle -= deltatime;
        if(spirit->timetonextparticle <= 0)
        {
            spirit->timetonextparticle = spirit->timebetweenparticles;
            Particle particle = spirittrailparticle;
            double distance = spirit->size / 5;
            particle.x = randdouble(spirit->x - distance, spirit->x + distance);
            particle.y = randdouble(spirit->y - distance, spirit->y + distance);
            particle.w = randdouble(spirit->size, spirit->size * 1.3);
            particle.h = randdouble(spirit->size, spirit->size * 1.3);
            particle.angle = randdouble(0, 90);
            particle.angvel = randint(-50, 50);
            particle.vely = -10;
            particle.color = spirit->color;
            particle.drawpriority = 0;
            addparticle(&mainps, particle);

            distance = spirit->size / 3;
            particle.x = randdouble(spirit->x - distance, spirit->x + distance);
            particle.y = randdouble(spirit->y - distance, spirit->y + distance);
            particle.w /= 1.4;
            particle.h /= 1.4;
            particle.angle = randdouble(0, 90);
            particle.angvel = randint(-50, 50);
            particle.vely = -10;
            particle.color = color_white;
            particle.drawpriority = 1;
            addparticle(&mainps, particle);
        }
    }
}

void
updateparticles(ParticleSystem *ps)
{
    for(int i = 0; i < ps->numparticles; i++)
    {
        ps->particles[i].x += ps->particles[i].velx * deltatime;
        ps->particles[i].y += ps->particles[i].vely * deltatime;
        ps->particles[i].w += ps->particles[i].delw * deltatime;
        ps->particles[i].h += ps->particles[i].delh * deltatime;
        ps->particles[i].angle += ps->particles[i].angvel * deltatime;
        ps->particles[i].alpha += ps->particles[i].delalpha * deltatime;
        ps->particles[i].timeleft -= deltatime;
        advanceanimation(&(ps->particles[i].anim), deltatime);

        if(ps->particles[i].diewhensmall)
        {
            if(ps->particles[i].w <= 0 || ps->particles[i].h <= 0 || ps->particles[i].alpha <= 0)
            {
                ps->particles[i] = ps->particles[ps->numparticles - 1];
                ps->numparticles -= 1;
            }
        }
        else
        {
            if(ps->particles[i].timeleft <= 0)
            {
                ps->particles[i] = ps->particles[ps->numparticles - 1];
                ps->numparticles -= 1;
            }
        }
    }
}



Vectori
getemptymapspace(bool** map)
{
    int count = 0;
    Vectori vector;
    do
    {
        vector.x = randint(1, mapw-1);
        vector.y = randint(1, maph-1);
    } while(map[vector.x][vector.y] && count < 10);
    // TODO(aidan): implement a more reliable method
    return vector;
}

int
render(Camera cam)
{
    SDL_SetRenderDrawColor(cam.renderer,
                           color_grass_green.r,
                           color_grass_green.g,
                           color_grass_green.b,
                           color_grass_green.a);
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

    drawmap(cam, mainmap, mapw, maph);

    drawparticles(cam, &mainps);

    // draw player
    SDL_Rect playerrect = recttoscreen(cam,
                                       playerx - playerw/2.0,
                                       playery - playerh/2.0,
                                       playerw,
                                       playerh,
                                       space_game);
    SDL_SetRenderDrawColor(cam.renderer, 255, 0, 255, 255);
    //SDL_RenderFillRect(cam.renderer, &playerrect);
    SDL_RenderCopy(cam.renderer,
                   currentplayeranim->frames[currentplayeranim->currentframe],
                   NULL,
                   &playerrect);
    if(debug.colliders)
    {
        SDL_SetRenderDrawColor(cam.renderer,
                               debug.collidercolor.r,
                               debug.collidercolor.g,
                               debug.collidercolor.b,
                               debug.collidercolor.a);
        drawrect(cam,
                 playerx - playerw / 2,
                 playery - playerh / 2,
                 playerw,
                 playerh);
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
                                          space_game);

        advanceanimation(&(spirit->anim), deltatime);
        SDL_RenderCopy(cam.renderer,
                       spirit->anim.frames[spirit->anim.currentframe],
                       NULL,
                       &spiritrect);

        // draw colliders if debug option enabled
        if(debug.colliders)
        {
            SDL_SetRenderDrawColor(cam.renderer,
                                   debug.collidercolor.r,
                                   debug.collidercolor.g,
                                   debug.collidercolor.b,
                                   debug.collidercolor.a);
            drawrect(cam,
                    spirit->x - spirit->size / 2,
                    spirit->y - spirit->size / 2,
                    spirit->size,
                    spirit->size);
        }

        // highlight selected spirit
        if(spirit == selectedspirit)
        {
            SDL_SetRenderDrawColor(cam.renderer,
                                   color_blue.r,
                                   color_blue.g,
                                   color_blue.b,
                                   color_blue.a);
            drawrect(cam,
                    spirit->x - spirit->size / 2,
                    spirit->y - spirit->size / 2,
                    spirit->size,
                    spirit->size);
        }
    }

    // draw ui
    if(debugMenuEnabled)
    {
        UIDocker debugdocker;
        debugdocker.x = cam.wui*-1/2;
        debugdocker.y = cam.hui*-1/2;
        debugdocker.totaloffset = 0;
        debugdocker.vertical = false;
        debugdocker.offsetscalar = 1.1;
        if(drawbuttoncheckbox(cam, &debugdocker, "grid", 5, debug.worldgrid))
        {
            debug.worldgrid = !debug.worldgrid;
        }
        if(drawbuttoncheckbox(cam, &debugdocker, "colliders", 5, debug.colliders))
        {
            debug.colliders = !debug.colliders;
        }
        if(drawbuttoncheckbox(cam, &debugdocker, "freecam", 5, debug.freecam))
        {
            debug.freecam = !debug.freecam;
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


    if(cam.wres != cam.hres)
    {
        drawsidebars(cam);
    }
    SDL_RenderPresent(cam.renderer);
}

int
updatespiritgrabbing()
{
    if(selectedspirit != NULL)
    {
        if(selectedspirit->state == state_beingheld)
        {
            if(keysup.z)
            {
                printf("throwing spirit!\n");
                // throw spirit
                selectedspirit->state = state_thrown;
                selectedspirit->velx = playervelx*2;
                selectedspirit->vely = playervely*2;
                selectedspirit = NULL;
            }
        }
        else
        {
            if(keysup.z)
            {
                selectedspirit->state = state_beingheld;
            }
        }
    }
}

int
updateplayermovement()
{
    Vectorf poschange = {0};
    double zoomchange = 0;
    if(keysdown.up)
        poschange.y -= deltatime;
    if(keysdown.down)
        poschange.y += deltatime;
    if(keysdown.left)
        poschange.x -= deltatime;
    if(keysdown.right)
        poschange.x += deltatime;
    if(keysdown.z)
        zoomchange -= camerazoomspeed * deltatime;
    if(keysdown.x)
        zoomchange += camerazoomspeed * deltatime;

    if(debug.freecam)
    {
        poschange.x *= cameramovespeed;
        poschange.y *= cameramovespeed;
        maincam.w += zoomchange * maincam.w;
        maincam.h += zoomchange * maincam.h;
        maincam.x += poschange.x * maincam.w * deltatime;
        maincam.y += poschange.y * maincam.h * deltatime;
        if(maincam.w < maincam.minw)
            maincam.w = maincam.minw;
        else if(maincam.w > maincam.maxw)
            maincam.w = maincam.maxw;
        if(maincam.h < maincam.minh)
            maincam.h = maincam.minh;
        else if(maincam.h > maincam.maxh)
            maincam.h = maincam.maxh;
    }
    else
    {
        // ABANDON ALL HOPE, YE WHO ENTER HERE,
        // FOR THIS IS THE CODE OF DEAD MEN
        poschange.x *= playeracceleration;
        poschange.y *= playeracceleration;
        bool freezex = false;
        bool freezey = false;
        playervelx += poschange.x;
        playervely += poschange.y;

        if(playervelx > 0)
        {
            playervelx -= playerdrag;
            if(playervelx < 0)
            {
                playervelx = 0;
            }
        }
        else if(playervelx < 0)
        {
            playervelx += playerdrag;
            if(playervelx > 0)
            {
                playervelx = 0;
            }
        }
        if(playervely > 0)
        {
            playervely -= playerdrag;
            if(playervely < 0)
            {
                playervely = 0;
            }
        }
        else if(playervely < 0)
        {
            playervely += playerdrag;
            if(playervely > 0)
            {
                playervely = 0;
            }
        }

        if(playervelx > playermaxspeed)
            playervelx = playermaxspeed;
        else if (playervelx < -playermaxspeed)
            playervelx = -playermaxspeed;
        if(playervely > playermaxspeed)
            playervely = playermaxspeed;
        else if (playervely < -playermaxspeed)
            playervely = -playermaxspeed;

        // Nowmawize vectow  > w <
        if(poschange.x != 0 || poschange.y != 0)
        {
            double length = sqrt((playervelx*playervelx) + (playervely*playervely));
            if(length != 0)
            {
                // Not sure why these if-statements are needed,
                // but without them the player slides along the wall
                // if holding the button towards the wall
                if(poschange.x != 0)
                {
                    playervelx /= length;
                    playervelx *= playermaxspeed;
                }
                if(poschange.y != 0)
                {
                    playervely /= length;
                    playervely *= playermaxspeed;
                }
            }
        }

        for(int x = 0; x < mapw; x++)
        {
            for(int y = 0; y < maph; y++)
            {
                double obstaclex = x*mapsquaresize;
                double obstacley = y*mapsquaresize;
                if(mainmap[x][y])
                {
                    if(willcollidex(playerx, playery, playerw, playerh, playervelx * deltatime,
                                    obstaclex, obstacley, mapsquaresize, mapsquaresize, 0))
                    {
                        playervelx = 0;
                        if(playerx > obstaclex)
                            playerx = obstaclex + mapsquaresize/2.0 + playerw/2.0;
                        else
                            playerx = obstaclex - mapsquaresize/2.0 - playerw/2.0;
                    }
                    if(willcollidey(playerx, playery, playerw, playerh, playervely * deltatime,
                                    obstaclex, obstacley, mapsquaresize, mapsquaresize, 0))
                    {
                        playervely = 0;
                        if(playery > obstacley)
                            playery = obstacley + mapsquaresize/2.0 + playerh/2.0;
                        else
                            playery = obstacley - mapsquaresize/2.0 - playerh/2.0;
                    }
                }
            }
        }
        if(freezex)
        {
            playervelx = 0;
            if(!freezey)
            {
                if(playervely > 0)
                {
                    playerdir = dir_down;
                }
                else if(playervely < 0)
                {
                    playerdir = dir_up;
                }
            }
        }
        if(freezey)
        {
            playervely = 0;
            if(!freezex)
            {
                if(playervelx > 0)
                {
                    playerdir = dir_right;
                }
                else if(playervelx < 0)
                {
                    playerdir = dir_left;
                }
            }
        }
        if(playervelx == 0 && playervely == 0)
        {
            if(poschange.y > 0)
            {
                playerdir = dir_down;
            }
            else if(poschange.y < 0)
            {
                playerdir = dir_up;
            }
            else if(poschange.x > 0)
            {
                playerdir = dir_right;
            }
            else if(poschange.x < 0)
            {
                playerdir = dir_left;
            }
        }
        if(!freezex && !freezey && (playervelx != 0 || playervely != 0))
        {
            if(abs(playervely) >= 0.1)
            {
                if(playervely > 0)
                {
                    playerdir = dir_down;
                }
                else
                {
                    playerdir = dir_up;
                }
            }
            else
            {
                if(playervelx > 0)
                {
                    playerdir = dir_right;
                }
                else
                {
                    playerdir = dir_left;
                }
            }
        }

        currentplayeranim = &playeranims[playerdir];
        for(int i = 0; i < PLAYERANIMATIONCOUNT; i++)
        {
            if(i != playerdir)
            {
                playeranims[i].currentframe = 0;
                playeranims[i].timetillnext = playeranims[i].timetillnext;
            }
        }

        if(playervelx != 0 || playervely != 0)
        {
            advanceanimation(currentplayeranim, deltatime);
        }
        else
        {
            for(int i = 0; i < PLAYERANIMATIONCOUNT; i++)
            {
                playeranims[i].currentframe = 0;
                playeranims[i].timetillnext = playeranims[i].timetillnext;
            }
        }

        playerx += playervelx * deltatime;
        playery += playervely * deltatime;

        double lastcamx = maincam.x;
        double lastcamy = maincam.y;
        maincam.x = interpolatelinear(maincam.x, playerx, 0.03);
        maincam.y = interpolatelinear(maincam.y, playery, 0.03);
    }
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

    debug.collidercolor.r = color_yellow.r;
    debug.collidercolor.g = color_yellow.g;
    debug.collidercolor.b = color_yellow.b;
    debug.collidercolor.a = color_yellow.a;
    debug.colliders = false;
    debug.worldgridcolor.r = 0;
    debug.worldgridcolor.g = 0;
    debug.worldgridcolor.b = 0;
    debug.worldgridcolor.a = 255;
    debug.worldgridgapsize = 20;
    debug.worldgrid = false;
    debug.freecam = false;
    debugMenuEnabled = true;

    mapw = 10;
    maph = 10;
    mapsquaresize = 25;
    mainmap = (bool**)malloc(mapw*sizeof(bool*));
    for(int i = 0; i < mapw; i++)
    {
        mainmap[i] = (bool*)malloc(maph*sizeof(bool));
        for(int a = 0; a < maph; a++)
        {
            mainmap[i][a] = randint(0, 5) == 0;
            if(i == 0 || i == mapw-1 || a == 0 || a == maph-1)
            {
                mainmap[i][a] = true;
            }
        }
    }

    maincam.x = (mapw-1)*mapsquaresize/2;
    maincam.y = (maph-1)*mapsquaresize/2;
    //maincam.w = mapw*mapsquaresize / 1.5;
    //maincam.h = maph*mapsquaresize / 1.5;
    maincam.w = maincam.h = 235;
    maincam.wui = maincam.hui = 100;
    maincam.minw = maincam.minh = 40; // TODO(aidan): find out why values below 40
                                      // cause world grid rendering bugs
    maincam.maxw = maincam.w * 50.0;
    maincam.maxh = maincam.h * 50.0;
    maincam.wres = 500;
    maincam.hres = 500;

    mousex = maincam.x + maincam.w / 2;
    mousey = maincam.y + maincam.h / 2;

    Vectori playerinitpos = getemptymapspace(mainmap);
    playerx = playerinitpos.x * mapsquaresize;
    playery = playerinitpos.y * mapsquaresize;

    maincam.x = playerx;
    maincam.y = playery;

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

            loadtextures(renderer);

            for(int i = 0; i < PLAYERANIMATIONCOUNT; i++)
            {
                playeranims[i].numframes = 4;
                playeranims[i].currentframe = 0;
                playeranims[i].frametime = 0.1;
                playeranims[i].timetillnext = playeranims[i].frametime;
            }

            particle1anim.frames[0] = particle1textures[0];
            particle1anim.numframes = 1;
            particle1anim.currentframe = 0;
            particle1anim.frametime = 0.5;
            particle1anim.timetillnext = particle1anim.frametime;

            mainps.numparticles = 0;
            spirittrailparticle = makeparticle(particle1anim);
            spirittrailparticle.w = 10;
            spirittrailparticle.h = 10;
            spirittrailparticle.delw = -8;
            spirittrailparticle.delh = -8;
            spirittrailparticle.angle = 45;
            spirittrailparticle.angvel = 1;
            spirittrailparticle.alpha = 255;
            spirittrailparticle.delalpha = -300;
            spirittrailparticle.timeleft = 5;
            spirittrailparticle.diewhensmall = true;
            spirittrailparticle.color = color_white;
            spirittrailparticle.anim = particle1anim;

            for(int i = 0; i < 1; i++)
            {
                Vectori pos = getemptymapspace(mainmap);
                Spirit *newspirit;

                newspirit = makespirit(pos.x, pos.y, 8, color_orange);

                newspirit->timebetweenparticles = 0.1;
                newspirit->timetonextparticle = 0;
                int left = randint(0, 2);
                int up = randint(0, 2);
                double speed = 15;
                if(left == 0)
                    newspirit->velx = -speed;
                else
                    newspirit->velx = speed;
                if(up == 0)
                    newspirit->vely = -speed;
                else
                    newspirit->vely = speed;
            }

            int gameupdatehz = 144;
            float targetsecondsperframe = 1.0f / (float)gameupdatehz;
            int lastcounter = SDL_GetPerformanceCounter();
            deltatime = 0;
            bool running = true;
            while(running)
            {
                SDL_Event event;
                mouseleftup = false;
                keysup.z = false;
                while(SDL_PollEvent(&event))
                {
                    if(handleevent(&event))
                    {
                        running = false;
                    }
                }

                updateplayermovement();
                updatespiritgrabbing();
                updatespirits();
                updateparticles(&mainps);
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

                deltatime = counterelapsed / 1000000000.0;

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
                keysup.z = true;
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
