#include <stdio.h>
#include <SDL/SDL.h>
#include <math.h>

#include <supervisory.h>
#include <timer.h>

#define PERIOD_MS 50

#define SCREEN_W 640 // window resolution
#define SCREEN_H 480

#define Y_MAX 110
#define Y_MAX_DRAW 100
#define X_MAX 60

#define BPP 32
typedef Uint32 PixelType;

typedef struct canvas
{
    SDL_Surface *canvas;
    int Height;  // canvas height
    int Width;   // canvas width
    int Xoffset; // X off set, in canvas pixels
    int Yoffset; // Y off set, in canvas pixels
    int Xext;    // X extra width
    int Yext;    // Y extra height
    double Xmax;
    double Ymax;
    double Xstep; // half a distance between X pixels in 'Xmax' scale

    PixelType *zpixel;

} Tcanvas;

typedef struct dataholder
{
    Tcanvas *canvas;
    double Tcurrent;
    double Lcurrent;
    PixelType Lcolor;
    double INcurrent;
    PixelType INcolor;
    double OUTcurrent;
    PixelType OUTcolor;

} Tdataholder;

extern inline void c_pixeldraw(Tcanvas *canvas, int x, int y, PixelType color)
{
    *(((PixelType *)canvas->canvas->pixels) + ((-y + canvas->Yoffset) * canvas->canvas->w + x + canvas->Xoffset)) = color;
}

extern inline void c_hlinedraw(Tcanvas *canvas, int xstep, int y, PixelType color)
{
    int offset = (-y + canvas->Yoffset) * canvas->canvas->w;
    int x;

    for (x = 0; x < canvas->Width + canvas->Xoffset; x += xstep)
    {
        *(((PixelType *)canvas->canvas->pixels) + (offset + x)) = color;
    }
}

extern inline void c_vlinedraw(Tcanvas *canvas, int x, int ystep, PixelType color)
{
    int offset = x + canvas->Xoffset;
    int y;
    int Ystep = ystep * canvas->canvas->w;

    for (y = 0; y < canvas->Height + canvas->Yext; y += ystep)
    {
        *(((PixelType *)canvas->canvas->pixels) + (offset + y * canvas->canvas->w)) = color;
    }
}

extern inline void c_linedraw(Tcanvas *canvas, double x0, double y0, double x1, double y1, PixelType color)
{
    double x;

    for (x = x0; x <= x1; x += canvas->Xstep)
    {
        c_pixeldraw(canvas, (int)(x * canvas->Width / canvas->Xmax + 0.5), (int)((double)canvas->Height / canvas->Ymax * (y1 * (x1 - x) + y1 * (x - x0)) / (x1 - x0) + 0.5), color);
    }
}

Tcanvas *c_open(int Width, int Height, double Xmax, double Ymax)
{
    int x, y;
    Tcanvas *canvas;
    canvas = malloc(sizeof(Tcanvas));

    canvas->Xoffset = 10;
    canvas->Yoffset = Height;

    canvas->Xext = 10;
    canvas->Yext = 10;

    canvas->Height = Height;
    canvas->Width = Width;
    canvas->Xmax = Xmax;
    canvas->Ymax = Ymax;

    canvas->Xstep = Xmax / (double)Width / 2;

    //  canvas->zpixel = (PixelType *)canvas->canvas->pixels +(Height-1)*canvas->canvas->w;

    SDL_Init(SDL_INIT_VIDEO); // SDL init
    canvas->canvas = SDL_SetVideoMode(canvas->Width + canvas->Xext, canvas->Height + canvas->Yext, BPP, SDL_SWSURFACE);

    c_hlinedraw(canvas, 1, 0, (PixelType)SDL_MapRGB(canvas->canvas->format, 255, 255, 255));
    for (y = 10; y < Ymax; y += 10)
    {
        c_hlinedraw(canvas, 3, y * Height / Ymax, (PixelType)SDL_MapRGB(canvas->canvas->format, 220, 220, 220));
    }
    c_vlinedraw(canvas, 0, 1, (PixelType)SDL_MapRGB(canvas->canvas->format, 255, 255, 255));
    for (x = 10; x < Xmax; x += 10)
    {
        c_vlinedraw(canvas, x * Width / Xmax, 3, (PixelType)SDL_MapRGB(canvas->canvas->format, 220, 220, 220));
    }

    return canvas;
}

void *datainit(Tdataholder* data, int Width, int Height, double Xmax, double Ymax, double Lcurrent, double INcurrent, double OUTcurrent)
{
    data->canvas = c_open(Width, Height, Xmax, Ymax);
    data->Tcurrent = 0;
    data->Lcurrent = Lcurrent;
    data->Lcolor = (PixelType)SDL_MapRGB(data->canvas->canvas->format, 255, 180, 0);
    data->INcurrent = INcurrent;
    data->INcolor = (PixelType)SDL_MapRGB(data->canvas->canvas->format, 180, 255, 0);
    data->OUTcurrent = OUTcurrent;
    data->OUTcolor = (PixelType)SDL_MapRGB(data->canvas->canvas->format, 0, 180, 255);
}

void setdatacolors(Tdataholder *data, PixelType Lcolor, PixelType INcolor, PixelType OUTcolor)
{
    data->Lcolor = Lcolor;
    data->INcolor = INcolor;
    data->OUTcolor = OUTcolor;
}

void datadraw(Tdataholder *data, double time, double level, double inangle, double outangle)
{
    c_linedraw(data->canvas, data->Tcurrent, data->Lcurrent, time, level, data->Lcolor);
    c_linedraw(data->canvas, data->Tcurrent, data->INcurrent, time, inangle, data->INcolor);
    c_linedraw(data->canvas, data->Tcurrent, data->OUTcurrent, time, outangle, data->OUTcolor);
    data->Tcurrent = time;
    data->Lcurrent = level;
    data->INcurrent = inangle;
    data->OUTcurrent = outangle;

    SDL_Flip(data->canvas->canvas);
}

void quitevent()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            // close files, etc...
            SDL_Quit();
            exit(EXIT_SUCCESS); // this will terminate all threads !
        }
    }
}

void *supervisory(void *sdptr) {
    Tdataholder data;

    SupervisoryData* supervisoryData = (SupervisoryData*)sdptr;
    double level;
    double angleIn;
    double angleOut;

    struct timespec tStart, t;
    double deltaS;

    datainit(&data, SCREEN_W, SCREEN_H, X_MAX, Y_MAX, 0, 0, 0);
    datadraw(&data, 0, 0, 0, 0);

    puts("Supervisory initialized, waiting simulation start");
    while (!(*(supervisoryData->started))) quitevent();

    getCurrentTime(&tStart);
    t = tStart;

    puts("Supervisory started");
    while (1) {
        deltaS = ((double)getPassedTimeMs(&tStart)) / 1000;
        if (deltaS > X_MAX) {
            datainit(&data, SCREEN_W, SCREEN_H, X_MAX, Y_MAX, 0, 0, 0);
            getCurrentTime(&tStart);
            deltaS = ((double)getPassedTimeMs(&tStart)) / 1000;
        }

        pthread_mutex_lock(supervisoryData->levelLock);
        level = *(supervisoryData->level);
        pthread_mutex_unlock(supervisoryData->levelLock);

        pthread_mutex_lock(supervisoryData->angleLock);
        angleIn = *(supervisoryData->angleIn);
        angleOut = *(supervisoryData->angleOut);
        pthread_mutex_unlock(supervisoryData->angleLock);

        level = fmax(0, fmin(Y_MAX_DRAW, level*100));
        angleIn = fmax(0, fmin(Y_MAX_DRAW, angleIn));
        angleOut = fmax(0, fmin(Y_MAX_DRAW, angleOut));

        datadraw(&data, deltaS, level, angleIn, angleOut);

        while (getPassedTimeMs(&t) < PERIOD_MS) quitevent();
        getCurrentTime(&t);
    }
}
