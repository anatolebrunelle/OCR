#ifndef PREPROCESSING_H
#define PREPROCESSING_H

#include <SDL2/SDL.h>

SDL_Surface *preprocess_grayscale(const SDL_Surface *input);
SDL_Surface *preprocess_binarize(const SDL_Surface *input);
SDL_Surface *preprocess_rotate(const SDL_Surface *input, double degrees);

#endif
