#include <stdio.h>
#include <stdlib.h>
#include "../include/raylib.h"
#include "types.h"

#define SCREEN_WIDTH    800
#define SCREEN_HEIGHT   450
#define FPS             60


#define USAGE_ERROR     1
#define UNKNOWN_OPCODE  2

#define MAX_GAME_SIZE   4096 /* 4 KB */
#define START_MEMORY    0x200 /* 512 */

u8 V[16]; // 16 8-bit registers, from V0 to VF.
u8 VF = V[0xF]; // Special register used as a flag. It is not used by the programs.

u16 I; // Address register.
u16 stack[16];

u8 sp; // Stack pointer.
u16 pc; // Program counter.

u8 dt; // Delay timer.
u8 st; // Sound timer.

// Each font is made of 5 8-bit values (1 byte for each row), ranging from 0 to F.
u8 fonts[5 * 16] = {
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

/* Reference: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#3.1 */
void simulate(u8 *game_buffer)
{
    while (1) {
        u16 opcode = *(game_buffer + pc) << 8 | *(game_buffer + pc + 1);
        pc += 2;

        printf("Simulating opcode: %02x\n", opcode);

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
                // TODO: should I set the collision or is it done by the program?
            } break;
            case 0x2000: { // 2nnn: Call subroutine at nnn.
                stack[sp++] = pc;
                pc = opcode & 0xFFF;
            } break;
            default: {
                fprintf(stderr, "Unknown opcode: %02x\n", opcode);

                exit(UNKNOWN_OPCODE);
            } break;
        }
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

    // InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Title");

    // SetTargetFPS(FPS);

    u8 *game_buffer = (u8 *)malloc(MAX_GAME_SIZE);
    FILE *game;
    errno_t fopen_error = fopen_s(&game, filename_game, "rb");
    if (fopen_error == 0) {
        while (!feof(game)) {
            size_t bytes_read = fread(game_buffer, 1, MAX_GAME_SIZE, game);
            // for (int i = 0; i < bytes_read; i++) {
            //     printf("%02x ", game_buffer[i]);
            // }
        }
    }

    // pc = START_MEMORY;
    pc = 0;
    sp = 0;
    
    simulate(game_buffer);
    
    free(game_buffer);


    // Main loop
    // while (!WindowShouldClose()) {

    //     // Do the work here!

    //     BeginDrawing();
    //         ClearBackground(RAYWHITE);
    //         DrawText("Congrats! You created your first window!", 190, 200, 20, BLACK);
    //     EndDrawing();
    // }

    // CloseWindow();
    
    return 0;
}
