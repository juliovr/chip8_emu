#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/raylib.h"
#include "types.h"

#define SCREEN_WIDTH    800
#define SCREEN_HEIGHT   450
#define FPS             60


#define USAGE_ERROR     1
#define UNKNOWN_OPCODE  2

#define MAX_MEMORY_SIZE 4096 /* 4 KB */
#define START_MEMORY    0x200 /* First 512 are reserved */

u8 V[16]; // 16 8-bit registers, from V0 to VF.
u8 VF = V[0xF]; // Special register used as a flag. It is not used by the programs.

u16 I; // Address register.
u16 stack[16];

u8 sp; // Stack pointer.
u16 pc; // Program counter.

u8 dt; // Delay timer.
u8 st; // Sound timer.

u8 *memory;

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
void simulate(u8 *memory)
{
    while (1) {
        u16 opcode = *(memory + pc) << 8 | *(memory + pc + 1);
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
                // TODO: should I set the collision or is it done by the program?
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

                    default: {
                        fprintf(stderr, "Unknown opcode: %04x\n", opcode);

                        exit(UNKNOWN_OPCODE);
                    } break;
                }
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

    memory = (u8 *)malloc(MAX_MEMORY_SIZE);
    memset(memory, 0, MAX_MEMORY_SIZE);

    FILE *game;
    errno_t fopen_error = fopen_s(&game, filename_game, "rb");
    if (fopen_error == 0) {
        while (!feof(game)) {
            size_t bytes_read = fread(memory + START_MEMORY, 1, MAX_MEMORY_SIZE, game);
            // for (int i = 0; i < bytes_read; i++) {
            //     printf("%02x ", game_buffer[i]);
            // }
        }
    }

    pc = START_MEMORY;
    sp = 0;
    
    simulate(memory);
    
    free(memory);


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
