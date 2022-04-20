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

void
loadfonts()
{
    TTF_Init();
    debugfont = TTF_OpenFont("resources/fonts/JetBrainsMono-Medium.ttf", 90);
    if(debugfont == NULL)
        printf("Font could not be loaded!\n");
}

void
loadtextures(SDL_Renderer *renderer)
{
    spirittextures[0] = texturefromimage(renderer, "resources/graphics/spirit0.png");
    spirittextures[1] = texturefromimage(renderer, "resources/graphics/spirit1.png");
    particle1textures[0] = texturefromimage(renderer, "resources/graphics/particle1.png");
    playeranims[0].frames[0] = texturefromimage(renderer, "resources/graphics/playerdown0.png");
    playeranims[0].frames[1] = texturefromimage(renderer, "resources/graphics/playerdown1.png");
    playeranims[0].frames[2] = texturefromimage(renderer, "resources/graphics/playerdown2.png");
    playeranims[0].frames[3] = texturefromimage(renderer, "resources/graphics/playerdown3.png");

    playeranims[1].frames[0] = texturefromimage(renderer, "resources/graphics/playerup0.png");
    playeranims[1].frames[1] = texturefromimage(renderer, "resources/graphics/playerup1.png");
    playeranims[1].frames[2] = texturefromimage(renderer, "resources/graphics/playerup2.png");
    playeranims[1].frames[3] = texturefromimage(renderer, "resources/graphics/playerup3.png");

    playeranims[2].frames[0] = texturefromimage(renderer, "resources/graphics/playerleft0.png");
    playeranims[2].frames[1] = texturefromimage(renderer, "resources/graphics/playerleft1.png");
    playeranims[2].frames[2] = texturefromimage(renderer, "resources/graphics/playerleft2.png");
    playeranims[2].frames[3] = texturefromimage(renderer, "resources/graphics/playerleft3.png");

    playeranims[3].frames[0] = texturefromimage(renderer, "resources/graphics/playerright0.png");
    playeranims[3].frames[1] = texturefromimage(renderer, "resources/graphics/playerright1.png");
    playeranims[3].frames[2] = texturefromimage(renderer, "resources/graphics/playerright2.png");
    playeranims[3].frames[3] = texturefromimage(renderer, "resources/graphics/playerright3.png");
}
