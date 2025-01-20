#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <emscripten.h>

uint8_t clamp(int value)
{
    return value > 255 ? 255 : value < 0 ? 0
                                         : value;
}

void grayscale(uint8_t *imageData, int width, int height)
{
    int size = width * height * 4; // Assuming RGBA format (4 bytes per pixel)
    for (int i = 0; i < size; i += 4)
    {
        uint8_t r = imageData[i];
        uint8_t g = imageData[i + 1];
        uint8_t b = imageData[i + 2];
        // grayscale weights: R * 0.3 + G * 0.59 + B * 0.11
        uint8_t gray = (uint8_t)(r * 0.3 + g * 0.59 + b * 0.11);

        imageData[i] = gray;
        imageData[i + 1] = gray;
        imageData[i + 2] = gray;
    }
}

void sepia(uint8_t *imageData, int width, int height)
{
    int size = width * height * 4;
    for (int i = 0; i < size; i += 4)
    {
        uint8_t r = imageData[i];
        uint8_t g = imageData[i + 1];
        uint8_t b = imageData[i + 2];

        // important - these must be cast as int.
        // uint8_t will wrap values and the clamp will not work as expected
        int sepiaR = (int)(r * 0.393 + g * 0.769 + b * 0.189);
        int sepiaG = (int)(r * 0.349 + g * 0.686 + b * 0.168);
        int sepiaB = (int)(r * 0.272 + g * 0.534 + b * 0.131);

        imageData[i] = clamp(sepiaR);
        imageData[i + 1] = clamp(sepiaG);
        imageData[i + 2] = clamp(sepiaB);
    }
}

// 1/(2*pi*sigma^2) * e^(-(x^2+y^2)/(2*sigma^2))
float gaussianKernel(int x, int y, int sigma)
{
    float coefficient = 1.0f / (2.0f * M_PI * sigma * sigma);
    float exponent = -(x * x + y * y) / (2.0f * sigma * sigma);
    return coefficient * exp(exponent);
}

void createGaussianKernel(float *kernel, int kernelSize, int sigma)
{
    int halfSize = kernelSize / 2;
    float sum = 0.0f;

    for (int y = -halfSize; y <= halfSize; y++)
    {
        for (int x = -halfSize; x <= halfSize; x++)
        {
            float value = gaussianKernel(x, y, sigma);
            kernel[(y + halfSize) * kernelSize + (x + halfSize)] = value;
            sum += value;
        }
    }

    // Normalize the kernel
    for (int i = 0; i < kernelSize * kernelSize; i++)
    {
        kernel[i] /= sum;
    }
}

void gaussianBlur(uint8_t *imageData, int width, int height, int kernelSize, int sigma)
{
    int halfSize = kernelSize / 2;
    float *kernel = malloc(kernelSize * kernelSize * sizeof(float));
    createGaussianKernel(kernel, kernelSize, sigma);
    // TODO
}

void mosaic(uint8_t *imageData, int width, int height, int blockSize)
{
    // subdivide image into blocks of size blockSize
    for (int y = 0; y < height; y += blockSize)
    {
        for (int x = 0; x < width; x += blockSize)
        {
            int rSum = 0, gSum = 0, bSum = 0, count = 0;

            // sum and average rgb values of all pixels in the block
            for (int by = y; by < y + blockSize && by < height; by++)
            {
                for (int bx = x; bx < x + blockSize && bx < width; bx++)
                {
                    int index = (by * width + bx) * 4;
                    rSum += imageData[index];
                    gSum += imageData[index + 1];
                    bSum += imageData[index + 2];
                    count++;
                }
            }

            uint8_t rAvg = rSum / count;
            uint8_t gAvg = gSum / count;
            uint8_t bAvg = bSum / count;

            // Set every pixel in block to the average color
            for (int by = y; by < y + blockSize && by < height; by++)
            {
                for (int bx = x; bx < x + blockSize && bx < width; bx++)
                {
                    int index = (by * width + bx) * 4;
                    imageData[index] = rAvg;
                    imageData[index + 1] = gAvg;
                    imageData[index + 2] = bAvg;
                }
            }
        }
    }
}

void hueRotation(uint8_t *imageData, int width, int height, float angle)
{
    // TODO
}

void dropShadow(uint8_t *imageData, int width, int height, int offsetX, int offsetY, uint8_t shadowColor[3])
{
    // TODO
}

void emboss(uint8_t *imageData, int width, int height)
{
    // TODO
}

void kuwahara(uint8_t *imageData, int width, int height)
{
    // TODO
}

void invert(uint8_t *imageData, int width, int height)
{
    for (int i = 0; i < width * height * 4; i += 4)
    {
        imageData[i] = 255 - imageData[i];
        imageData[i + 1] = 255 - imageData[i + 1];
        imageData[i + 2] = 255 - imageData[i + 2];
    }
}

// Expose the function to JavaScript
EMSCRIPTEN_KEEPALIVE
void apply_grayscale(uint8_t *imageData, int width, int height)
{
    grayscale(imageData, width, height);
}

EMSCRIPTEN_KEEPALIVE
void apply_sepia(uint8_t *imageData, int width, int height)
{
    sepia(imageData, width, height);
}

EMSCRIPTEN_KEEPALIVE
void apply_gaussian(uint8_t *imageData, int width, int height, int kernelSize, int sigma)
{
    gaussianBlur(imageData, width, height, kernelSize, sigma);
}

EMSCRIPTEN_KEEPALIVE
void apply_mosaic(uint8_t *imageData, int width, int height, int blockSize)
{
    mosaic(imageData, width, height, blockSize);
}

EMSCRIPTEN_KEEPALIVE
void apply_invert(uint8_t *imageData, int width, int height)
{
    invert(imageData, width, height);
}

// emcc image_filter.c -o image_filter.js -s EXPORTED_FUNCTIONS='["_apply_grayscale", "_apply_sepia", "_apply_mosaic", "_apply_invert", "_malloc", "_free"]' -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "HEAPU8"]' -s WASM=1 -s INITIAL_MEMORY=33554432
