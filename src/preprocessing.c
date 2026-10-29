#include "../include/preprocessing.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

static SDL_Surface *copy_rgba(const SDL_Surface *input)
{
    if (!input || input->w <= 0 || input->h <= 0)
        return NULL;
    return SDL_ConvertSurfaceFormat((SDL_Surface *)input,
                                    SDL_PIXELFORMAT_RGBA32, 0);
}

static Uint32 get_pixel(const SDL_Surface *surface, int x, int y)
{
    const Uint8 *row = (const Uint8 *)surface->pixels + y * surface->pitch;
    return ((const Uint32 *)row)[x];
}

static void set_pixel(SDL_Surface *surface, int x, int y, Uint32 value)
{
    Uint8 *row = (Uint8 *)surface->pixels + y * surface->pitch;
    ((Uint32 *)row)[x] = value;
}

static Uint8 luminance(Uint8 r, Uint8 g, Uint8 b)
{
    return (Uint8)((299u * r + 587u * g + 114u * b + 500u) / 1000u);
}

SDL_Surface *preprocess_grayscale(const SDL_Surface *input)
{
    SDL_Surface *output = copy_rgba(input);
    if (!output)
        return NULL;
    if (SDL_MUSTLOCK(output) && SDL_LockSurface(output) != 0)
    {
        SDL_FreeSurface(output);
        return NULL;
    }
    for (int y = 0; y < output->h; y++)
        for (int x = 0; x < output->w; x++)
        {
            Uint8 r, g, b, a;
            SDL_GetRGBA(get_pixel(output, x, y), output->format,
                        &r, &g, &b, &a);
            Uint8 gray = luminance(r, g, b);
            gray = (Uint8)((gray * a + 255u * (255u - a)) / 255u);
            set_pixel(output, x, y, SDL_MapRGBA(output->format,
                                                gray, gray, gray, 255));
        }
    if (SDL_MUSTLOCK(output))
        SDL_UnlockSurface(output);
    return output;
}

SDL_Surface *preprocess_binarize(const SDL_Surface *input)
{
    SDL_Surface *output = preprocess_grayscale(input);
    if (!output)
        return NULL;
    if (SDL_MUSTLOCK(output) && SDL_LockSurface(output) != 0)
    {
        SDL_FreeSurface(output);
        return NULL;
    }

    uint64_t histogram[256] = { 0 };
    uint64_t sum = 0;
    for (int y = 0; y < output->h; y++)
        for (int x = 0; x < output->w; x++)
        {
            Uint8 r, g, b;
            SDL_GetRGB(get_pixel(output, x, y), output->format, &r, &g, &b);
            histogram[r]++;
            sum += r;
        }

    uint64_t total = (uint64_t)output->w * (uint64_t)output->h;
    uint64_t weight = 0, partial_sum = 0;
    double best = -1.0;
    int threshold = 127;
    for (int t = 0; t < 256; t++)
    {
        weight += histogram[t];
        partial_sum += (uint64_t)t * histogram[t];
        if (weight == 0 || weight == total)
            continue;
        double left = (double)partial_sum / (double)weight;
        double right = (double)(sum - partial_sum) / (total - weight);
        double delta = left - right;
        double score = (double)weight * (double)(total - weight)
                       * delta * delta;
        if (score > best)
        {
            best = score;
            threshold = t;
        }
    }

    for (int y = 0; y < output->h; y++)
        for (int x = 0; x < output->w; x++)
        {
            Uint8 r, g, b;
            SDL_GetRGB(get_pixel(output, x, y), output->format, &r, &g, &b);
            Uint8 value = r <= threshold ? 0 : 255;
            set_pixel(output, x, y, SDL_MapRGB(output->format,
                                               value, value, value));
        }
    if (SDL_MUSTLOCK(output))
        SDL_UnlockSurface(output);
    return output;
}

SDL_Surface *preprocess_rotate(const SDL_Surface *input, double degrees)
{
    if (!isfinite(degrees))
        return NULL;
    SDL_Surface *source = copy_rgba(input);
    if (!source)
        return NULL;

    double radians = remainder(degrees, 360.0) * acos(-1.0) / 180.0;
    double cs = cos(radians), sn = sin(radians);
    double width = fabs(cs) * source->w + fabs(sn) * source->h;
    double height = fabs(sn) * source->w + fabs(cs) * source->h;
    if (width > 100000 || height > 100000)
    {
        SDL_FreeSurface(source);
        return NULL;
    }
    int out_w = (int)ceil(width - 1e-9);
    int out_h = (int)ceil(height - 1e-9);
    SDL_Surface *output = SDL_CreateRGBSurfaceWithFormat(
        0, out_w, out_h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!output)
    {
        SDL_FreeSurface(source);
        return NULL;
    }
    SDL_FillRect(output, NULL, SDL_MapRGB(output->format, 255, 255, 255));

    if ((SDL_MUSTLOCK(source) && SDL_LockSurface(source) != 0)
        || (SDL_MUSTLOCK(output) && SDL_LockSurface(output) != 0))
    {
        if (SDL_MUSTLOCK(source) && source->locked)
            SDL_UnlockSurface(source);
        SDL_FreeSurface(source);
        SDL_FreeSurface(output);
        return NULL;
    }

    double source_cx = (source->w - 1) / 2.0;
    double source_cy = (source->h - 1) / 2.0;
    double output_cx = (out_w - 1) / 2.0;
    double output_cy = (out_h - 1) / 2.0;
    for (int y = 0; y < out_h; y++)
        for (int x = 0; x < out_w; x++)
        {
            double dx = x - output_cx, dy = y - output_cy;
            double sx = cs * dx - sn * dy + source_cx;
            double sy = sn * dx + cs * dy + source_cy;
            int ix = (int)floor(sx + 0.5);
            int iy = (int)floor(sy + 0.5);
            if (ix < 0 || iy < 0 || ix >= source->w || iy >= source->h)
                continue;
            Uint8 r, g, b, a;
            SDL_GetRGBA(get_pixel(source, ix, iy), source->format,
                        &r, &g, &b, &a);
            r = (Uint8)((r * a + 255u * (255u - a)) / 255u);
            g = (Uint8)((g * a + 255u * (255u - a)) / 255u);
            b = (Uint8)((b * a + 255u * (255u - a)) / 255u);
            set_pixel(output, x, y, SDL_MapRGB(output->format, r, g, b));
        }
    if (SDL_MUSTLOCK(output))
        SDL_UnlockSurface(output);
    if (SDL_MUSTLOCK(source))
        SDL_UnlockSurface(source);
    SDL_FreeSurface(source);
    return output;
}
