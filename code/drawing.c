void
drawparticle(Camera cam, Particle particle)
{
    SDL_Rect rect = recttoscreen(cam,
                                 particle.x - particle.w/2,
                                 particle.y - particle.h/2,
                                 particle.w,
                                 particle.h,
                                 space_game);
    SDL_Point center;
    center.x = rect.x;
    center.y = rect.y;
    //SDL_SetTextureBlendMode(particle.anim.frames[particle.anim.currentframe],
    //                        SDL_BLENDMODE_ADD);
    SDL_SetTextureColorMod(particle.anim.frames[particle.anim.currentframe],
                           particle.color.r,
                           particle.color.g,
                           particle.color.b);
    SDL_SetTextureAlphaMod(particle.anim.frames[particle.anim.currentframe],
                           particle.alpha);

    //SDL_SetTextureBlendMode(particle.anim.frames[particle.anim.currentframe],
    //                        particle.blendmode);

    SDL_RenderCopyEx(cam.renderer,
                     particle.anim.frames[particle.anim.currentframe],
                     NULL,
                     &rect,
                     particle.angle,
                     NULL,
                     //&center,
                     SDL_FLIP_NONE);

    SDL_SetTextureBlendMode(particle.anim.frames[particle.anim.currentframe],
                            SDL_BLENDMODE_BLEND);
    SDL_SetTextureColorMod(particle.anim.frames[particle.anim.currentframe],
                           255,
                           255,
                           255);
}

void
drawparticles(Camera cam, ParticleSystem *ps)
{
    for(int i = 0; i < ps->numparticles; i++)
    {
        if(ps->particles[i].drawpriority == 0)
        {
            drawparticle(cam, ps->particles[i]);
        }
    }
    for(int i = 0; i < ps->numparticles; i++)
    {
        if(ps->particles[i].drawpriority == 1)
        {
            drawparticle(cam, ps->particles[i]);
        }
    }
}

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
        SDL_Rect screenrect = recttoscreen(cam, x, y, 0, h, space_ui);
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

    SDL_Rect rectscreen = recttoscreen(cam, x, y, w, h, space_ui);
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


    SDL_Rect rectscreen = recttoscreen(cam, x, y, w, h, space_ui);
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
    SDL_Rect rect = recttoscreen(cam, x, y, w, h, space_game);
    SDL_RenderDrawRect(cam.renderer, &rect);
}

int
drawmap(Camera cam, bool** map, int w, int h)
{
    SDL_SetRenderDrawColor(cam.renderer,
                           155,
                           155,
                           155,
                           255);
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
                                             space_game);

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
