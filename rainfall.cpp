#include <string.h>
#include <math.h>
#include <stdlib.h>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif
#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"

int* gFrameBuffer;
SDL_Window* gSDLWindow;
SDL_Renderer* gSDLRenderer;
SDL_Texture* gSDLTexture;
static int gDone;
const int WINDOW_WIDTH = 1920 / 2;
const int WINDOW_HEIGHT = 1080 / 2;

// return false here to quit the application
bool update()
{
    SDL_Event e;
    if (SDL_PollEvent(&e))
    {
        if (e.type == SDL_EVENT_QUIT)
        {
            return false;
        }
        if (e.type == SDL_EVENT_KEY_UP && e.key.key == SDLK_ESCAPE)
        {
            return false;
        }
    }

    char* pix;
    int pitch;

    // First, we lock the texture for write access, getting pointer to the texture data as well as pitch.
    // Pitch being the amount of bytes in a horizontal strip of the texture. May not match the actual texture width in some cases, though in this case it does.
    SDL_LockTexture(gSDLTexture, NULL, (void**)&pix, &pitch);

    // memcpy just copies memory from one location to another
    // in this case its copying the frame buffer to the texture
    for (int i = 0, sp = 0, dp = 0; i < WINDOW_HEIGHT; i++, dp += WINDOW_WIDTH, sp += pitch)
        memcpy(pix + sp, gFrameBuffer + dp, WINDOW_WIDTH * 4);

    SDL_UnlockTexture(gSDLTexture); // unlock texture to make SDL3 copy the texture data to VRAM
    SDL_RenderTexture(gSDLRenderer, gSDLTexture, NULL, NULL); // ask sdl to render the texture to the target
    SDL_RenderPresent(gSDLRenderer); // present the render target to the screen
    SDL_Delay(1); // let the cpu rest for a bit (shouldn't affect fps at all)

    // Since the texture update is a write-only operation, and we might not update the whole display every frame, we need to have our separate framebuffer array.
    return true;
}

// Pixel formatted into ABGR8888 (aka 32 bits total with 8 for alpha, blue, green, red respectively)
// ts should be a shader ngl vro
void putpixel(int x, int y, int color)
{
    if (x < 0 ||
        y < 0 ||
        x >= WINDOW_WIDTH ||
        y >= WINDOW_HEIGHT)
    {
        return;
    }
    gFrameBuffer[y * WINDOW_WIDTH + x] = color;
}

const unsigned char sprite[] =
{
    0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,
    0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,
    0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
    0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
    0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,
    0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0
};


void drawsprite(int x, int y, unsigned int color)
{
    int i, j, c, yofs;
    yofs = y * WINDOW_WIDTH + x;
    for (i = 0, c = 0; i < 16; i++)
    {
        for (j = 0; j < 16; j++, c++)
        {
            if (sprite[c])
            {
                gFrameBuffer[yofs + j] = color;
            }
        }
        yofs += WINDOW_WIDTH;
    }
}

void drawcloud(int x, int y, int displacement, unsigned int color)
{
    drawsprite(x, (int)(y - 1 * displacement), color);
    drawsprite((int)(x + (sqrt(3) / 2) * displacement), (int)(y + (-1 / 2) * displacement), color);
    drawsprite((int)(x - (sqrt(3) / 2) * displacement), (int)(y + (-1 / 2) * displacement), color);
}

void init()
{
    // black out screen
    for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
        gFrameBuffer[i] = 0xff000000;

    // generate terrain through stacked sines
    for (int i = 0; i < WINDOW_WIDTH; i++)
    {
        int p = (int)((sin((i + 3247) * 0.02) * 0.3 +
            sin((i + 2347) * 0.04) * 0.1 +
            sin((i + 4378) * 0.01) * 0.6) * 100 + (WINDOW_HEIGHT * 2 / 3));
        int pos = p * WINDOW_WIDTH + i;
        for (int j = p; j < WINDOW_HEIGHT; j++)
        {
            gFrameBuffer[pos] = 0xff007f00;
            pos += WINDOW_WIDTH; // adding WINDOW_WIDTH moves one layer down
        }
    }

    drawcloud((int)(WINDOW_WIDTH / 2), (int)(WINDOW_HEIGHT / 6), 8, 0xff879ba3);
    drawcloud((int)(WINDOW_WIDTH / 3), (int)(WINDOW_HEIGHT / 6), 8, 0xff879ba3);
    drawcloud((int)(2 * WINDOW_WIDTH / 3), (int)(WINDOW_HEIGHT / 6), 8, 0xff879ba3);
}


void cloudcover(Uint64 aTicks)
{
    // black out cloud pixels
    for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
        if (gFrameBuffer[i] == 0xff879ba3)
            gFrameBuffer[i] = 0xff000000;

    // draw clouds
    //drawcloud((int)(WINDOW_WIDTH / 2), (int)(WINDOW_HEIGHT / 6), 8, 0xff879ba3);
    //drawcloud((int)(WINDOW_WIDTH / 3), (int)(WINDOW_HEIGHT / 6), 8, 0xff879ba3);
    //drawcloud((int)(2 * WINDOW_WIDTH / 3), (int)(WINDOW_HEIGHT / 6), 8, 0xff879ba3);

    for (int i = 1; i <= 5; i++)
    {
        int time = (int)(sqrt(i) * aTicks / 20);

        int positionX = (int)(WINDOW_WIDTH / 20) + (time % (int)(18 * WINDOW_WIDTH / 20));
        int positionY = i * (int)(WINDOW_HEIGHT / 15);

        drawcloud(positionX, positionY, 8, 0xff879ba3);
    }

    // new snow under clouds
    for (int j = WINDOW_HEIGHT - 2; j >= 0; j--)
    {
        int ypos = j * WINDOW_WIDTH;
        for (int i = 1; i < WINDOW_WIDTH - 1; i++)
        {
            if (gFrameBuffer[ypos + i] != 0xff879ba3)
                continue;

            if (gFrameBuffer[ypos + i + WINDOW_WIDTH] != 0xff000000)
                continue;

            if (rand() % 30 != 0)
                continue;

            gFrameBuffer[ypos + i + WINDOW_WIDTH] = 0xffff5511;
        }
    }
}

void rainfall()
{
    // cellular automata
    // for every pixel on the screen:
    // check if the pixel is white
    // if so check if the pixel below is black, if not continue
    // if pixel is black set current pixel to black and the below pixel to white
    // this is a really neat snowfall/powder algorithm actually
    // so like if the below pixel is green or white nothing happens so it just stacks up
    // thats rlly cool actually
    // also it piles up and checks if it canm ove to the sides if it cant move down too
    for (int j = WINDOW_HEIGHT - 2; j >= 0; j--)
    {
        int ypos = j * WINDOW_WIDTH;
        for (int i = 1; i < WINDOW_WIDTH - 1; i++)
        {
            if (gFrameBuffer[ypos + i] != 0xffff5511)
                continue;

            if (gFrameBuffer[ypos + i + WINDOW_WIDTH] != 0xff000000 &&
                gFrameBuffer[ypos + i + WINDOW_WIDTH] != 0xff879ba3)
                continue;

            gFrameBuffer[ypos + i + WINDOW_WIDTH] = 0xffff5511;
            gFrameBuffer[ypos + i] = 0xff000000;
        }
    }

    // seperate iteration so that the rule can apply without weird artifacts
    for (int j = WINDOW_HEIGHT - 2; j >= 0; j--)
    {
        int ypos = j * WINDOW_WIDTH;
        for (int i = 1; i < WINDOW_WIDTH - 1; i++)
        {
            if (gFrameBuffer[ypos + i] != 0xffff5511)
                continue;

            if (gFrameBuffer[ypos + i + WINDOW_WIDTH] == 0xff000000 ||
                gFrameBuffer[ypos + i + WINDOW_WIDTH] == 0xff879ba3)
                continue;

            if (gFrameBuffer[ypos + i + WINDOW_WIDTH - 1] == 0xff000000 ||
                gFrameBuffer[ypos + i + WINDOW_WIDTH - 1] == 0xff879ba3)
            {
                gFrameBuffer[ypos + i + WINDOW_WIDTH - 1] = 0xffff5511;
                gFrameBuffer[ypos + i] = 0xff000000;
            }
            else if (gFrameBuffer[ypos + i + WINDOW_WIDTH + 1] == 0xff000000 ||
                gFrameBuffer[ypos + i + WINDOW_WIDTH + 1] == 0xff879ba3)
            {
                gFrameBuffer[ypos + i + WINDOW_WIDTH + 1] = 0xffff5511;
                gFrameBuffer[ypos + i] = 0xff000000;
            }
            else if (gFrameBuffer[ypos + i + WINDOW_WIDTH - 2] == 0xff000000 ||
                gFrameBuffer[ypos + i + WINDOW_WIDTH - 2] == 0xff879ba3)
            {
                gFrameBuffer[ypos + i + WINDOW_WIDTH - 2] = 0xffff5511;
                gFrameBuffer[ypos + i] = 0xff000000;
            }
            else if (gFrameBuffer[ypos + i + WINDOW_WIDTH + 2] == 0xff000000 ||
                gFrameBuffer[ypos + i + WINDOW_WIDTH + 2] == 0xff879ba3)
            {
                gFrameBuffer[ypos + i + WINDOW_WIDTH + 2] = 0xffff5511;
                gFrameBuffer[ypos + i] = 0xff000000;
            }
            else if (rand() % 120 == 0)
            {
                gFrameBuffer[ypos + i] = 0xff000000;
            }
        }
    }
}

// This is wht the tutorial will mostly be focusing on.
// It loops over every pixel and draws a pattern on the image.
void render(Uint64 aTicks)
{
    cloudcover(aTicks);
    rainfall();
}

void loop()
{
    if (!update())
    {
        gDone = 1; // actually exits the program if set to 1
#ifdef __EMSCRIPTEN__ // this enables building for the web btw just ignore it lma
        emscripten_cancel_main_loop();
#endif
    }
    else
    {
        render(SDL_GetTicks());
    }
}

int main(int argc, char** argv)
{
    // setup SDL3
    // if it fails it quits
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        return -1;
    }

    // setup frame buffer and stuff
    gFrameBuffer = new int[WINDOW_WIDTH * WINDOW_HEIGHT]; // Our framebuffer. Each integer in the array will represent one pixel on the screen.
    gSDLWindow = SDL_CreateWindow("SDL3 window", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    gSDLRenderer = SDL_CreateRenderer(gSDLWindow, NULL);
    gSDLTexture = SDL_CreateTexture(gSDLRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);

    // error checking/if these dont get setup fsr quit
    if (!gFrameBuffer || !gSDLWindow || !gSDLRenderer || !gSDLTexture)
        return -1;

    init();

    // loop until gdone is somethijng other than 0 
    // (booleans are just integer 0 and 1 btw in c and c++
    gDone = 0;
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(loop, 0, 1);
#else
    while (!gDone)
    {
        loop();
    }
#endif

    // quit everything !
    SDL_DestroyTexture(gSDLTexture);
    SDL_DestroyRenderer(gSDLRenderer);
    SDL_DestroyWindow(gSDLWindow);
    SDL_Quit();

    return 0;
}