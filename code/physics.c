#include <math.h>

typedef struct
BoxCollider {
    double x; double y;
    double w; double h;
} BoxCollider;

typedef struct
CircleCollider {
    double x; double y;
    double radius;
} CircleCollider;

typedef struct
Vectorf {
    double x; double y;
} Vectorf;

typedef struct
Vectori {
    int x; int y;
} Vectori;

typedef struct
physicsBody {
    Vectorf position;
    Vectorf velocity;
    double drag;
    BoxCollider collider;
} physicsBody;

    // amount should be between 0.0 and 1.0
double
interpolateLinear(double amount, double a, double b)
{
    int result = amount;
    result *= abs(a - b);
    if(a > b)
        result += b;
    else
        result += a;
    return result;
}

int
boxCollidersColliding(double x1, double y1, double w1, double h1, double x2, double y2, double w2, double h2)
{
    if(x1 + w1 > x2 ||
       x2 + w2 > x1 ||
       y1 + h1 > y2 ||
       y2 + h2 > y1)
        return 0;
    return 1;
}

Vectorf
boxColliderDifference(double x1, double y1, double w1, double h1, double x2, double y2, double w2, double h2)
{
    Vectorf diff;
    diff.x = x1 + w1 - x2;
    diff.x = y1 + h1 - y2;
    return diff;
}

int
pointInBoxCollider(double xpt, double ypt, double xbox, double ybox, double wbox, double hbox)
{
    if(xpt < xbox ||
       xpt > xbox + wbox ||
       ypt < ybox ||
       ypt > ybox + hbox)
        return 0;
    return 1;
}

int
boxAndCircleCollidersColliding(double x1, double y1, double radius, double x2, double y2, double w, double h)
{
        // get closest point to center of box
    Vectorf boxCenter;
    boxCenter.x = x2 + w / 2;
    boxCenter.y = y2 + h / 2;

    double angle = atan( fabs(boxCenter.x - x1) / fabs(boxCenter.y - y1) );

    double pointX = sin(angle) / radius;
    double pointY = cos(angle) / radius;

    return pointInBoxCollider(pointX, pointY, x2, y2, w, h);
}
