#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/raylib.h"
#include "types.h"

#define SCREEN_WIDTH    (64)
#define SCREEN_HEIGHT   (32)
#define SCREEN_SIZE     (SCREEN_WIDTH*SCREEN_HEIGHT)
#define SCALE           (40)    /* Pixel scale */
#define WINDOW_WIDTH    (SCREEN_WIDTH*SCALE)
#define WINDOW_HEIGHT   (SCREEN_HEIGHT*SCALE)
#define FPS             (16)


#define USAGE_ERROR     1
#define UNKNOWN_OPCODE  2

#define MAX_MEMORY_SIZE (4096)  /* 4 KB */
#define START_MEMORY    (0x200) /* First 512 are reserved */

u8 V[16]; // 16 8-bit registers, from V0 to VF.
u8 VF = V[0xF]; // Special register used as a flag. It is not used by the programs.

u16 I; // Address register.
u16 stack[16];

u8 sp; // Stack pointer.
u16 pc; // Program counter.

u8 dt; // Delay timer.
u8 st; // Sound timer.

u8 *memory;

u8 screen[SCREEN_SIZE];

// Each font is made of 5 8-bit values (1 byte for each row), ranging from 0 to F.
#define FONTS_MEMORY_SIZE   (5 * 16)
u8 fonts[FONTS_MEMORY_SIZE] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F
};

u16 opcode;

void clear_screen()
{
    memset(screen, 0, sizeof(screen));
}

/* Reference: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#3.1 */
void simulate(u8 *memory)
{
    opcode = *(memory + pc) << 8 | *(memory + pc + 1);
    pc += 2;

    printf("Simulating opcode: %04x\n", opcode);

    
    switch (opcode & 0xF000) {
        case 0x6000: { // 6xkk: Set Vx = kk.
            u8 register_index = (opcode >> 8) & 0xF;
            u8 value = opcode & 0xFF;
            V[register_index] = value;
        } break;
        case 0xA000 : { // Annn: Set I = nnn.
            I = opcode & 0xFFF;
        } break;
        case 0xD000: { // Dxyn: Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
            u8 sprite_width = 8;
            u8 n = opcode & 0xF;
            u8 x = V[(opcode >> 8) & 0xF];
            u8 y = V[(opcode >> 4) & 0xF];

            u8 collision = 0;

            for (int row = 0; row < n; row++) {
                u8 sprite_row = *(memory + I + row); // Each bit is 1 pixel

                for (int col = 0; col < sprite_width; col++) {
                    int index = ((y + row) * SCREEN_WIDTH) + (x + col);
                    u8 bit_value = (sprite_row >> (7 - col)) & 1; // Set the corresponding bit to the screen byte

                    if (screen[index] != bit_value) {
                        collision = 1;
                    }

                    screen[index] ^= bit_value;
                }
            }

            VF = collision;
        } break;
        case 0x1000: { // 1nnn: Jump to location nnn.
            pc = opcode & 0xFFF;
        } break;
        case 0x2000: { // 2nnn: Call subroutine at nnn.
            stack[sp++] = pc;
            pc = opcode & 0xFFF;
        } break;
        case 0x3000: { // 3xkk: Skip next instruction if Vx = kk.
            u8 vx = V[(opcode & 0xF00) >> 8];
            u8 kk = opcode & 0xFF;
            if (vx == kk) {
                pc += 2;
            }
        } break;
        case 0x7000: { // 7xkk: Set Vx = Vx + kk.
            V[(opcode & 0xF00) >> 8] += (opcode & 0xFF);
        } break;
        case 0xF000: {
            switch (opcode & 0xFF) {
                case 0x33: { // Fx33: Store BCD representation of Vx in memory locations I, I+1, and I+2.
                             // Takes the decimal value of Vx, and places the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.
                    u8 value = V[(opcode & 0xF00) >> 8];
                    *(memory + I) = value / 100;
                    value = value % 100;
                    *(memory + I + 1) = value / 10;
                    value = value % 10;
                    *(memory + I + 2) = value;
                } break;
            }
        } break;
        default: {
            switch (opcode & 0xFF) {
                case 0x00EE: { // 00EE: Return from a subroutine.
                    pc = stack[--sp];
                } break;

                case 0x00E0: {
                    clear_screen();
                } break;

                default: {
                    fprintf(stderr, "Unknown opcode: %04x\n", opcode);

                    exit(UNKNOWN_OPCODE);
                } break;
            }
        } break;
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <game>\n", argv[0]);

        exit(USAGE_ERROR);
    }

    char *filename_game = argv[1];
    printf("Loading %s...\n", filename_game);

    clear_screen();

    memory = (u8 *)malloc(MAX_MEMORY_SIZE);
    memset(memory, 0, MAX_MEMORY_SIZE);

    // Load font into memory
    for (int i = 0; i < FONTS_MEMORY_SIZE; i++) {
        memory[i] = fonts[i];
    }
    

    FILE *game;
    errno_t fopen_error = fopen_s(&game, filename_game, "rb");
    if (fopen_error == 0) {
        while (!feof(game)) {
            fread(memory + START_MEMORY, 1, MAX_MEMORY_SIZE, game);
        }
    }

    pc = START_MEMORY;
    sp = 0;

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, filename_game);

    SetTargetFPS(FPS);

    // Main loop
    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();

        simulate(memory);

        BeginDrawing();
            // ClearBackground(BLACK);

            // Draw pixels scaled
            for (int i = 0; i < SCREEN_HEIGHT; i++) {
                for (int j = 0; j < SCREEN_WIDTH; j++) {
                    int index = (i * SCREEN_WIDTH) + j;
                    Color color = (screen[index] == 1) ? WHITE : BLACK;
                    float x = (float)j*SCALE;
                    float y = (float)i*SCALE;
                    float width = SCALE;
                    float height = SCALE;

                    Rectangle pixel = { x, y, width, height };
                    DrawRectangleRec(pixel, color);
                }
            }


            char text[32];
            sprintf_s(text, "Simulating %04x", opcode);
            DrawText(text, 10, 10, 50, WHITE);
        EndDrawing();
    }

    free(memory);

    CloseWindow();
    
    return 0;
}
