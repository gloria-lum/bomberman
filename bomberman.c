#include "bomberman.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <stdio.h>

static void bomberman_game_mode_init(game_mode_t *game_mode)
{
    game_mode->timer = 60;
}

static void bomberman_map_init(cell_t *map)
{
}

static void bomberman_player_init(player_t *player)
{
    player->position.x = 0;
    player->position.y = 0;
    player->number_of_lifes = 1;
    player->number_of_bombs = 1;
    player->score = 0;
    player->speed = 1;
}

int bomberman_graphics_init(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **texture)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0)
    {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return -1;
    }

    *window = SDL_CreateWindow("SDL is active!", 100, 100, 512, 512, 0);
    if (!*window)
    {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!*renderer)
    {
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return -1;
    }

    int width;
    int height;
    int channels;
    unsigned char *pixels = stbi_load("pinguin.png", &width, &height, &channels, 4);
    if (!pixels)
    {
        SDL_Log("Unable to open image");
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return -1;
    }

    SDL_Log("Image width: %d height: %d channels: %d", width, height, channels);

    *texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    if (!*texture)
    {
        SDL_Log("Unable to create texture: %s", SDL_GetError());
        free(pixels);
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return -1;
    }

    SDL_UpdateTexture(*texture, NULL, pixels, width * 4);
    SDL_SetTextureAlphaMod(*texture, 255);
    SDL_SetTextureBlendMode(*texture, SDL_BLENDMODE_BLEND);
    free(pixels);
    return 0;
}

int set_nb(int s)
{
#ifdef _WIN32
    unsigned long nb_mode = 1;
    return ioctlsocket(s, FIONBIO, &nb_mode);
#else
    int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0)
        return flags;
    flags |= O_NONBLOCK;
    return fcntl(s, F_SETFL, flags);
#endif
}

int main(int argc, char **argv)
{

    game_mode_t game_mode;
    cell_t map[64 * 64];
    player_t player;

    bomberman_game_mode_init(&game_mode);

    bomberman_map_init(map);

    bomberman_player_init(&player);

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;

    if (bomberman_graphics_init(&window, &renderer, &texture))
    {
        return -1;
    }

    // game loop
    int running = 1;

    while (running)
    {
#ifdef _WIN32

        WSADATA wsa_data;
        if (WSAStartup(0x0202, &wsa_data))
        {
            printf("unable to initialize winsock2 \n");
            return -1;
        }
#endif
        int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        set_nb(s);
        if (s < 0)
        {
            printf("unable to initialize the UDP socket \n");
            return -1;
        }
        printf("socket %d created \n", s);
        struct sockaddr_in sin;
        inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr); // this will create a big endian 32 bit address
        sin.sin_family = AF_INET;
        sin.sin_port = htons(9999);
        // converts 9999 to big endian
        if (bind(s, (struct sockaddr *)&sin, sizeof(sin)))
        {
            printf("unable to bind the UDP socket \n");
            return -1;
        }

        for (;;)
        {
            char buffer[4096];

            struct sockaddr_in sender_in;

            int sender_in_size = sizeof(sender_in);
            int len = recvfrom(s, buffer, 4096, 0, (struct sockaddr *)&sender_in, &sender_in_size);
            
            if (len > 0)
            {
                char addr_as_string[64];
                inet_ntop(AF_INET, &sender_in.sin_addr, addr_as_string, 64);
                printf("received %d bytes from %s:%d\n", len, addr_as_string, ntohs(sender_in.sin_port));
            }

            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_QUIT)
                {
                    running = 0;
                }
            }
            SDL_PumpEvents();
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            SDL_Rect target_rect = {player.position.x, player.position.y, 32, 32};
            SDL_RenderCopy(renderer, texture, NULL, &target_rect);

            SDL_RenderPresent(renderer);

        
            if (strcmp(buffer, "u") == 0)
             {
                 player.position.y -= player.speed;
             }
             else if (strcmp(buffer, "d") == 0)
             {
                 player.position.y += player.speed;
             }
             else if (strcmp(buffer, "l") == 0)
             {
                 player.position.x -= player.speed;
             }
             else if (strcmp(buffer, "r") == 0)
             {
                 player.position.x += player.speed;
             }
        }

        /* const Uint8 *keys = SDL_GetKeyboardState(NULL);

         player.position.x += keys[SDL_SCANCODE_RIGHT];
         player.position.x -= keys[SDL_SCANCODE_LEFT];
         player.position.y += keys[SDL_SCANCODE_DOWN];
         player.position.y -= keys[SDL_SCANCODE_UP];*/
    }
    SDL_Quit();
    return 0;
}