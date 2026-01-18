#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// Forward declarations
extern void init_renderer(uint8_t* framebuffer, int width, int height);
extern void render_frame(void);
extern void init_simulation(void);
extern void update_simulation(float dt);
extern void init_ui(void);
extern void render_ui(void);
extern void handle_mouse_click(int x, int y, int button);
extern void handle_key_press(int key);
extern void move_camera(int dx, int dy);
extern void init_font(const char *font_path);
extern void cleanup_font(void);

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    uint8_t* framebuffer;
    bool running;
} GameState;

GameState g_state = {0};

bool init_sdl(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return false;
    }

    g_state.window = SDL_CreateWindow(
        "RCT Clone - Assembly Edition",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!g_state.window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return false;
    }

    g_state.renderer = SDL_CreateRenderer(
        g_state.window, -1, SDL_RENDERER_ACCELERATED
    );
    if (!g_state.renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        return false;
    }

    g_state.texture = SDL_CreateTexture(
        g_state.renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );

    if (!g_state.texture) {
        printf("Texture creation failed: %s\n", SDL_GetError());
        return false;
    }

    // Allocate framebuffer
    g_state.framebuffer = (uint8_t*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
    if (!g_state.framebuffer) {
        printf("Framebuffer allocation failed\n");
        return false;
    }

    return true;
}

void handle_events(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                g_state.running = false;
                break;
            case SDL_MOUSEBUTTONDOWN:
                handle_mouse_click(
                    event.button.x, event.button.y, event.button.button
                );
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    g_state.running = false;
                } else if (event.key.keysym.sym == SDLK_UP) {
                    move_camera(0, -10);
                } else if (event.key.keysym.sym == SDLK_DOWN) {
                    move_camera(0, 10);
                } else if (event.key.keysym.sym == SDLK_LEFT) {
                    move_camera(-10, 0);
                } else if (event.key.keysym.sym == SDLK_RIGHT) {
                    move_camera(10, 0);
                } else {
                    handle_key_press(event.key.keysym.sym);
                }
                break;
        }
    }
}

void render(void) {
    // Clear framebuffer to sky blue
    uint32_t* pixels = (uint32_t*)g_state.framebuffer;
    uint32_t clear_color = 0xFF87D7FF;  // ARGB8888 format

    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        pixels[i] = clear_color;
    }

    // Render game world
    render_frame();

    // Render UI on top
    render_ui();

    // Update texture and present
    SDL_UpdateTexture(
        g_state.texture, NULL, g_state.framebuffer, SCREEN_WIDTH * 4
    );
    SDL_RenderCopy(g_state.renderer, g_state.texture, NULL, NULL);
    SDL_RenderPresent(g_state.renderer);
}

void cleanup(void) {
    cleanup_font();
    if (g_state.framebuffer) free(g_state.framebuffer);
    if (g_state.texture) SDL_DestroyTexture(g_state.texture);
    if (g_state.renderer) SDL_DestroyRenderer(g_state.renderer);
    if (g_state.window) SDL_DestroyWindow(g_state.window);
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("Initializing RCT Clone...\n");

    if (!init_sdl()) {
        cleanup();
        return 1;
    }

    // Initialize game systems
    init_renderer(g_state.framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT);
    init_simulation();
    init_ui();
    init_font("/home/de/workspace/rtc-clone/assets/sprites/funny-fmt.png");  // Initialize font ONCE at startup

    g_state.running = true;
    uint64_t last_time = SDL_GetPerformanceCounter();
    const double frequency = (double)SDL_GetPerformanceFrequency();

    printf("Game loop starting...\n");

    while (g_state.running) {
        uint64_t current_time = SDL_GetPerformanceCounter();
        float dt = (float)((current_time - last_time) / frequency);

        // Clamp delta time
        if (dt > 0.1f) dt = 0.1f;
        if (dt < 0.0001f) dt = 0.016f;

        last_time = current_time;

        handle_events();
        update_simulation(dt);
        render();

        // Target 60 FPS
        SDL_Delay(1);
    }

    printf("Shutting down...\n");
    cleanup();
    return 0;
}
