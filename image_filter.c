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
    // n x n float matrix
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

// TODO: output rgb is wrong
void hueRotation(uint8_t *imageData, int width, int height, float angle)
{
    int size = width * height * 4; // Assuming RGBA format (4 bytes per pixel)
    // Normalize angle to keep it in the range [0, 360]
    angle = fmod(angle, 360.0f);
    for (int i = 0; i < size; i += 4)
    {
        uint8_t r = imageData[i];
        uint8_t g = imageData[i + 1];
        uint8_t b = imageData[i + 2];

        float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f;
        float maxVal = fmax(rf, fmax(gf, bf));
        float minVal = fmin(rf, fmin(gf, bf));
        float delta = maxVal - minVal;
        float v = maxVal;
        float s = maxVal == 0 ? 0 : delta / maxVal;
        float h = 0.0f;
        if (delta != 0)
        {
            if (maxVal == rf)
            {
                h = (gf - bf) / delta;
            }
            else if (maxVal == gf)
            {
                h = 2 + (bf - rf) / delta;
            }
            else if (maxVal == bf)
            {
                h = 4 + (rf - gf) / delta;
            }
            h = h * 60;
            if (h < 0)
            {
                h += 360;
            }
        }

        h = fmod(h + angle, 360.0f);
        float c = (1 - fabs(2 * v - 1)) * s;
        float x = c * (1 - fabs(fmod(h / 60, 2) - 1));
        float m = v - c / 2;

        if (h < 60)
        {
            r = v;
            g = c + m;
            b = m;
        }
        else if (h < 120)
        {
            r = c + m;
            g = v;
            b = m;
        }
        else if (h < 180)
        {
            r = m;
            g = v;
            b = c + m;
        }
        else if (h < 240)
        {
            r = m;
            g = c + m;
            b = v;
        }
        else if (h < 300)
        {
            r = c + m;
            g = m;
            b = v;
        }
        else
        {
            r = v;
            g = m;
            b = c + m;
        }

        r = clamp((int)r * 255);
        g = clamp((int)g * 255);
        b = clamp((int)b * 255);
        imageData[i] = r;
        imageData[i + 1] = g;
        imageData[i + 2] = b;
    }
}

typedef struct
{
    float x, y, z;
} vec3;
float dot(vec3 a, vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
void saturation(uint8_t *imageData, int width, int height, int saturationFactor)
{
    int size = width * height * 4; // Assuming RGBA format (4 bytes per pixel)
    for (int i = 0; i < size; i += 4)
    {
        uint8_t r = imageData[i];
        uint8_t g = imageData[i + 1];
        uint8_t b = imageData[i + 2];

        float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f;
        vec3 luminance = {0.2126, 0.7152, 0.0722};
        vec3 rgb = {rf, gf, bf};

        float lum = dot(luminance, rgb);
        float newR = lum + saturationFactor * (rgb.x - lum);
        float newG = lum + saturationFactor * (rgb.y - lum);
        float newB = lum + saturationFactor * (rgb.z - lum);

        imageData[i] = clamp((int)(newR * 255));
        imageData[i + 1] = clamp((int)(newG * 255));
        imageData[i + 2] = clamp((int)(newB * 255));
    }
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

void brightness(uint8_t *imageData, int width, int height, int amount)
{
    // TODO
}

// Expose the function to JavaScript
EMSCRIPTEN_KEEPALIVE
void apply_grayscale(uint8_t *imageData, int width, int height) { grayscale(imageData, width, height); }

EMSCRIPTEN_KEEPALIVE
void apply_sepia(uint8_t *imageData, int width, int height) { sepia(imageData, width, height); }

EMSCRIPTEN_KEEPALIVE
void apply_gaussian(uint8_t *imageData, int width, int height, int kernelSize, int sigma) { gaussianBlur(imageData, width, height, kernelSize, sigma); }

EMSCRIPTEN_KEEPALIVE
void apply_mosaic(uint8_t *imageData, int width, int height, int blockSize) { mosaic(imageData, width, height, blockSize); }

EMSCRIPTEN_KEEPALIVE
void apply_invert(uint8_t *imageData, int width, int height) { invert(imageData, width, height); }

EMSCRIPTEN_KEEPALIVE
void apply_hueRotation(uint8_t *imageData, int width, int height, float angle) { hueRotation(imageData, width, height, angle); }

EMSCRIPTEN_KEEPALIVE
void apply_saturation(uint8_t *imageData, int width, int height, float saturationFactor) { saturation(imageData, width, height, saturationFactor); }
// emcc image_filter.c -o image_filter.js -s EXPORTED_FUNCTIONS='["_apply_grayscale", "_apply_hueRotation", "_apply_saturation", "_apply_sepia", "_apply_mosaic", "_apply_invert", "_malloc", "_free"]' -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "HEAPU8"]' -s WASM=1 -s ALLOW_MEMORY_GROWTH=1
// TODO: should image be downscaled before applying filters to reduce memory usage
