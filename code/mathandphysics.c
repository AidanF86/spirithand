#include <stdbool.h>

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
Camera {
    double x; double y;
    double w; double h; // in game units
    double minw; double minh;
    double maxw; double maxh;
    double wui; double hui;
    int wres; int hres;
    //double zoom;
    SDL_Renderer *renderer;
} Camera;

enum spaces {game, ui};

int randint(int min, int max)
{
    double randf = (double)(rand()) / (double)RAND_MAX;
    return (max - min) * randf + min;
}

double randdouble(double min, double max)
{
    double randf = (double)(rand()) / (double)RAND_MAX;
    return (max - min) * randf + min;
}

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
boxescolliding(double x1, double y1, double w1, double h1, double x2, double y2, double w2, double h2)
{
    if(x1 < x2 + w2 &&
       x1 + w1 > x2 &&
       y1 < y2 + h2 &&
       h1 + y1 > y2)
        return true;
    return false;
}

bool
boxtotallyinbox(double x1, double y1, double w1, double h1, double x2, double y2, double w2, double h2)
{
    if(x1 < x2 ||
       x1 + w1 > x2 + w2 ||
       y1 < y2 ||
       y1 + h1 > y2 + h2)
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
vectorfgametoscreen(Camera cam, double x, double y)
{
    Vectori posincam;
    if((double)(cam.w) / (double)(cam.h) > (double)(cam.wres) / (double)(cam.hres))
    {
        // is taller
        double screentocamratio = (double)cam.wres / (double)cam.w;
        double yoffset = (cam.hres - (screentocamratio * cam.h)) / 2;
        posincam.y = (y - cam.y + cam.h / 2) * screentocamratio;
        posincam.y += yoffset;
        posincam.x = (x - cam.x + cam.w / 2) / cam.w * cam.wres;
    }
    else if((double)(cam.w) / (double)(cam.h) < (double)(cam.wres) / (double)(cam.hres))
    {
        // is longer
        double screentocamratio = (double)cam.hres / (double)cam.h;
        double xoffset = (cam.wres - (screentocamratio * cam.w)) / 2;
        posincam.x = (x - cam.x + cam.w / 2) * screentocamratio;
        posincam.x += xoffset;
        posincam.y = (y - cam.y + cam.h / 2) / cam.h * cam.hres;
    }
    else
    {
        // is neither
        posincam.x = (x - cam.x + cam.w / 2) / cam.w * cam.wres;
        posincam.y = (y - cam.y + cam.h / 2) / cam.h * cam.hres;
    }
    return posincam;
}

Vectorf
vectorfscreentogame(Camera cam, double x, double y)
{
    // TODO(aidan)
    Vectorf posincam;
    return posincam;
}

SDL_Rect
boxtoscreen(Camera cam, double x, double y, double w, double h, enum spaces space)
{
    double spacex;
    double spacey;
    double spacew;
    double spaceh;
    if(space == game)
    {
        spacex = cam.x;
        spacey = cam.y;
        spacew = cam.w;
        spaceh = cam.h;
    }
    else if(space == ui)
    {
        spacex = 0;
        spacey = 0;
        spacew = cam.wui;
        spaceh = cam.hui;
    }
    SDL_Rect rectincam;
    if((double)(cam.w) / (double)(cam.h) > (double)(cam.wres) / (double)(cam.hres))
    {
        // is taller
        double screentocamratio = (double)cam.wres / (double)spacew;
        double yoffset = (cam.hres - (screentocamratio * spaceh)) / 2;
        rectincam.y = (y - spacey + spaceh / 2) * screentocamratio;
        rectincam.y += yoffset;
        rectincam.x = (x - spacex + spacew / 2) / spacew * cam.wres;
        rectincam.h = h * screentocamratio;
        rectincam.w = w / spacew * cam.wres;
    }
    else if((double)(cam.w) / (double)(cam.h) < (double)(cam.wres) / (double)(cam.hres))
    {
        // is longer
        double screentocamratio = (double)cam.hres / (double)spaceh;
        double xoffset = (cam.wres - (screentocamratio * spacew)) / 2;
        rectincam.x = (x - spacex + spacew / 2) * screentocamratio;
        rectincam.x += xoffset;
        rectincam.y = (y - spacey + spaceh / 2) / spaceh * cam.hres;
        rectincam.w = w * screentocamratio;
        rectincam.h = h / spaceh * cam.hres;
    }
    else
    {
        // is neither
        rectincam.x = (x - spacex + spacew / 2) / spacew * cam.wres;
        rectincam.y = (y - spacey + spaceh / 2) / spaceh * cam.hres;
        rectincam.w = w / spacew * cam.wres;
        rectincam.h = h / spaceh * cam.hres;
    }
    //printf("%d, %d, %d, %d\n", rectincam.x, rectincam.y, rectincam.w, rectincam.h);
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
    return box;
}

int drawsidebars(Camera cam)
{
    SDL_SetRenderDrawColor(cam.renderer, 0, 0, 0, 255);
    SDL_Rect bars[2];
    if((double)(cam.w) / (double)(cam.h) > (double)(cam.wres) / (double)(cam.hres))
    {
        // is taller
        double screentocamratio = (double)cam.wres / (double)cam.w;
        double yoffset = (cam.hres - (screentocamratio * cam.h)) / 2;
        double barheight = (cam.hres - (cam.h * screentocamratio)) / 2;
        bars[0].x = 0;
        bars[0].y = 0;
        bars[0].w = cam.wres;
        bars[0].h = barheight;
        bars[1].x = 0;
        bars[1].y = cam.hres - barheight;
        bars[1].w = cam.wres;
        bars[1].h = barheight;
            // TODO(aidan): maybe change this to SDL_RenderFillRects for better performance
    }
    else if((double)(cam.w) / (double)(cam.h) < (double)(cam.wres) / (double)(cam.hres))
    {
        // is longer
        double screentocamratio = (double)cam.hres / (double)cam.h;
        double xoffset = (cam.wres - (screentocamratio * cam.w)) / 2;
        double barwidth = (cam.wres - (cam.w * screentocamratio)) / 2;
        bars[0].x = 0;
        bars[0].y = 0;
        bars[0].w = barwidth;
        bars[0].h = cam.hres;
        bars[1].x = cam.wres - barwidth;
        bars[1].y = 0;
        bars[1].w = barwidth;
        bars[1].h = cam.hres;
            // TODO(aidan): maybe change this to SDL_RenderFillRects for better performance
    }
    SDL_RenderFillRects(cam.renderer, bars, 2);
}

