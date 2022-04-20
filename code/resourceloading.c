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
}
