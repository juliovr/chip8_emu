#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/raylib.h"
#include "types.h"

#define assert(text)    do { printf(text); *((int *)0) = 0; } while (0);

#define SCREEN_WIDTH    (64)
#define SCREEN_HEIGHT   (32)
#define SCREEN_SIZE     (SCREEN_WIDTH*SCREEN_HEIGHT)
#define SCALE           (40)    /* Pixel scale */
#define WINDOW_WIDTH    (SCREEN_WIDTH*SCALE)
#define WINDOW_HEIGHT   (SCREEN_HEIGHT*SCALE)
#define FPS             (60)


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
#define FONT_SIZE_BYTES     5
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

/*
CHIP-8 keypad:
╔═══╦═══╦═══╦═══╗
║ 1 ║ 2 ║ 3 ║ C ║
╠═══╬═══╬═══╬═══╣
║ 4 ║ 5 ║ 6 ║ D ║
╠═══╬═══╬═══╬═══╣
║ 7 ║ 8 ║ 9 ║ E ║
╠═══╬═══╬═══╬═══╣
║ A ║ 0 ║ B ║ F ║
╚═══╩═══╩═══╩═══╝
*/
#define KEY_NUMBER 16
u8 chip8_keys[KEY_NUMBER] = {
    0x1, 0x2, 0x3, 0xC,
    0x4, 0x5, 0x6, 0xD,
    0x7, 0x8, 0x9, 0xE,
    0xA, 0x0, 0xB, 0xF,
};

int input_keys[KEY_NUMBER] = {
    KEY_ONE,    KEY_TWO,    KEY_THREE,  KEY_FOUR,
    KEY_Q,      KEY_W,      KEY_E,      KEY_R,
    KEY_A,      KEY_S,      KEY_D,      KEY_F,
    KEY_Z,      KEY_X,      KEY_C,      KEY_V,
};

int get_key_pressed(int key)
{
    printf("get_key_pressed = %d\n", key);
    for (int i = 0; i < KEY_NUMBER; i++) {
        if (input_keys[i] == key) {
            return chip8_keys[i];
        }
    }

    return -1;
}

void clear_screen()
{
    memset(screen, 0, sizeof(screen));
}

/* Reference: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#3.1 */
void simulate(u8 *memory)
{
    u16 opcode = *(memory + pc) << 8 | *(memory + pc + 1);
    pc += 2;

    // printf("Simulating opcode: %04x\n", opcode);

    
    switch (opcode & 0xF000) {

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

        case 0x4000: { // 4xkk: Skip next instruction if Vx != kk.
            u8 vx = V[(opcode & 0xF00) >> 8];
            u8 kk = opcode & 0xFF;
            if (vx != kk) {
                pc += 2;
            }
        } break;

        case 0x5000: { // 5xy0: Skip next instruction if Vx = Vy.
            u8 vx = V[(opcode & 0xF00) >> 8];
            u8 vy = V[(opcode & 0xF0) >> 4];
            if (vx == vy) {
                pc += 2;
            }
        } break;

        case 0x6000: { // 6xkk: Set Vx = kk.
            u8 register_index = (opcode >> 8) & 0xF;
            u8 value = opcode & 0xFF;
            V[register_index] = value;
        } break;

        case 0x7000: { // 7xkk: Set Vx = Vx + kk.
            V[(opcode & 0xF00) >> 8] += (opcode & 0xFF);
        } break;

        case 0x8000: {
            u8 x = (opcode & 0xF00) >> 8;
            u8 y = (opcode & 0xF0) >> 4;

            switch (opcode & 0xF) {
                case 0x0: { // 8xy0: Set Vx = Vy.
                    V[x] = V[y];
                } break;

                case 0x1: { // 8xy1: Set Vx = Vx OR Vy.
                    V[x] = V[x] | V[y];
                } break;

                case 0x2: { // 8xy2: Set Vx = Vx AND Vy.
                    V[x] = V[x] & V[y];
                } break;

                case 0x3: { // 8xy3: Set Vx = Vx XOR Vy.
                    V[x] = V[x] ^ V[y];
                } break;

                case 0x4: { // 8xy4: Set Vx = Vx + Vy, set VF = carry.
                    V[x] = V[x] + V[y];

                    // TODO: check carry. I think this is correct
                    VF = (V[x] < V[y]); // Carry
                } break;

                case 0x5: { // 8xy5: Set Vx = Vx - Vy, set VF = NOT borrow.
                    VF = (V[x] > V[y]);

                    V[x] = V[x] - V[y];
                } break;

                case 0x6: { // 8xy6: Set Vx = Vx SHR 1.
                    VF = (V[x] & 1); // If least-significant bit is 1
                    
                    V[x] >>= 1;
                } break;

                case 0x7: { // 8xy7: Set Vx = Vy - Vx, set VF = NOT borrow.
                    VF = (V[y] > V[x]);

                    V[x] = V[y] - V[x];
                } break;

                case 0xE: { // 8xyE: Set Vx = Vx SHL 1.
                    VF = (V[x] & 0x80); // If most-significant bit is 1

                    V[x] <<= 1;
                } break;

            }
        } break;

        case 0x9000: { // 9xy0: Skip next instruction if Vx != Vy.
            u8 vx = V[(opcode & 0xF00) >> 8];
            u8 vy = V[(opcode & 0xF0) >> 4];
            if (vx != vy) {
                pc += 2;
            }
        } break;

        case 0xA000 : { // Annn: Set I = nnn.
            I = opcode & 0xFFF;
        } break;

        case 0xB000: { // Bnnn: Jump to location nnn + V0.
            pc = (opcode & 0xFFF) + V[0];
        } break;

        case 0xC000: { // Cxkk: Set Vx = random byte AND kk.
            u8 random = 0; // TODO: compute random number
            V[(opcode & 0xF00) >> 8] = random & (opcode & 0xFF);
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
                    int screen_index = ((y + row) * SCREEN_WIDTH) + (x + col);
                    u8 bit_value = (sprite_row >> (7 - col)) & 1; // Set the corresponding bit to the screen byte

                    if (screen[screen_index] != bit_value) {
                        collision = 1;
                    }

                    screen[screen_index] ^= bit_value;
                }
            }

            VF = collision;
        } break;
        
        case 0xE000: {
            int key_pressed = get_key_pressed(GetKeyPressed());
            int vx = V[(opcode & 0xF00) >> 8];

            switch (opcode & 0xFF) {
                case 0x9E: { // Ex9E: Skip next instruction if key with the value of Vx is pressed.
                    if (key_pressed == vx) {
                        pc += 2;
                    }
                } break;

                case 0xA1: { // ExA1: Skip next instruction if key with the value of Vx is not pressed.
                    if (key_pressed != vx) {
                        pc += 2;
                    }
                } break;
            }
        } break;
        
        case 0xF000: {
            u8 x = (opcode & 0xF00) >> 8;
            switch (opcode & 0xFF) {
                case 0x07: { // Fx07: Set Vx = delay timer value.
                    V[x] = dt;
                } break;

                case 0x0A: { // Fx0A: Wait for a key press, store the value of the key in Vx.
                    int key_pressed;
                    while ((key_pressed = GetKeyPressed()) == 0)
                        ;

                    printf("key_pressed = %d\n", key_pressed);
                } break;

                case 0x15: { // Fx15: Set delay timer = Vx.
                    dt = V[x];
                } break;

                case 0x18: { // Fx18: Set sound timer = Vx.
                    st = V[x];
                } break;

                case 0x1E: { // Fx1E: Set I = I + Vx.
                    I += V[x];
                } break;

                case 0x29: { // Fx29: Set I to the memory address of the sprite data corresponding to the hexadecimal digit stored in register VX.
                    u8 vx = V[(opcode & 0xF00) >> 8];
                    I = (vx * FONT_SIZE_BYTES);
                } break;

                case 0x33: { // Fx33: Store BCD representation of Vx in memory locations I, I+1, and I+2.
                             // Takes the decimal value of Vx, and places the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.
                    u8 vx = V[x];
                    *(memory + I) = vx / 100;
                    vx = vx % 100;
                    *(memory + I + 1) = vx / 10;
                    vx = vx % 10;
                    *(memory + I + 2) = vx;
                } break;

                case 0x55: { // Fx55: Store registers V0 through Vx in memory starting at location I.
                    for (int i = 0; i < x; i++) {
                        *(memory + I + i) = V[i];
                    }
                } break;

                case 0x65: { // Fx65: Read registers V0 through Vx from memory starting at location I.
                    for (int i = 0; i < x; i++) {
                        V[i] = *(memory + I + i);
                    }
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
        // float delta_time = GetFrameTime();

        if (dt > 0) {
            dt--;
        }

        simulate(memory);

        BeginDrawing();

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
        EndDrawing();
    }

    free(memory);

    CloseWindow();
    
    return 0;
}
