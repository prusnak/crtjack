#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <jack/jack.h>
#include <SDL.h>

const int SCREEN_WIDTH  = 640;
const int SCREEN_HEIGHT = 480;

jack_port_t *in_r, *in_g, *in_b;
jack_nframes_t samplerate;
SDL_Surface *surface;
uint32_t *pixels;

void ensure(bool cond, const char *func)
{
    if (cond) {
        return;
    }
    fprintf(stderr, "%s Error: %s\n", func, SDL_GetError());
    exit(1);
}

void jack_shutdown(void *arg)
{
    ensure(0, "jack_shutdown");
}

float clamp(float v)
{
    const float min = -1.0f;
    const float max = +1.0f;
    const float t = v < min ? min : v;
    return t > max ? max : t;
}

int jack_process(jack_nframes_t nframes, void *arg)
{
    static int x = 0, y = 0;
    jack_default_audio_sample_t *sr, *sg, *sb;
    uint8_t r, g, b;
    uint32_t c;
    sr = jack_port_connected(in_r) > 0 ? jack_port_get_buffer(in_r, nframes) : 0;
    sg = jack_port_connected(in_g) > 0 ? jack_port_get_buffer(in_g, nframes) : 0;
    sb = jack_port_connected(in_b) > 0 ? jack_port_get_buffer(in_b, nframes) : 0;
    for (int i = 0; i < nframes; i++) {

        r = sr ? abs(clamp(sr[i]) * 255) : 0;
        g = sg ? abs(clamp(sg[i]) * 255) : 0;
        b = sb ? abs(clamp(sb[i]) * 255) : 0;
        c = (r << 24) | (g << 16) | (b << 8) | 0xff;
        for (int j = 0; j < 256; j++) {
            pixels[x + y * surface->pitch / sizeof(uint32_t)] = c;
            x++;
            if (x >= SCREEN_WIDTH) {
                x = 0;
                y++;
            }
            if (y >= SCREEN_HEIGHT) {
                x = y = 0;
            }
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    jack_status_t status;
    jack_client_t *client = jack_client_open("crtjack", JackNoStartServer | JackUseExactName, &status, 0);
    ensure(0 != client, "jack_client_open");

    jack_set_process_callback(client, jack_process, 0);
    jack_on_shutdown(client, jack_shutdown, 0);

    samplerate = jack_get_sample_rate(client);

    in_r = jack_port_register(client, "in_r", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    in_g = jack_port_register(client, "in_g", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    in_b = jack_port_register(client, "in_b", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    ensure(0 != in_r, "jack_port_register");
    ensure(0 != in_g, "jack_port_register");
    ensure(0 != in_b, "jack_port_register");

    ensure(0 == SDL_Init(SDL_INIT_VIDEO), "SDL_Init");

    SDL_Window *win = SDL_CreateWindow("crtjack", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    ensure(0 != win, "SDL_CreateWindow");

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    ensure(0 != ren, "SDL_CreateRenderer");

    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);

    surface = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    ensure(0 != surface, "SDL_CreateRGBSurface");
    pixels = (uint32_t *)surface->pixels;

    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT); 
    ensure(0 != tex, "SDL_CreateTexture");
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_NONE);
    SDL_SetTextureAlphaMod(tex, 0);

    ensure(0 == jack_activate(client), "jack_activate");

    SDL_Event event;
    for (;;) {
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            break;
        }
        SDL_RenderClear(ren);
        SDL_UpdateTexture(tex, NULL, surface->pixels, surface->pitch);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    jack_client_close (client);
    
    return 0;
}
