#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/raylib.h"
#include "types.h"

#define SCREEN_WIDTH            (64)
#define SCREEN_HEIGHT           (32)
#define SCREEN_SIZE             (SCREEN_WIDTH*SCREEN_HEIGHT)
#define SCALE                   (40)    /* Pixel scale */
#define WINDOW_WIDTH            (SCREEN_WIDTH*SCALE)
#define WINDOW_HEIGHT           (SCREEN_HEIGHT*SCALE)
#define FPS                     (240)

#define USAGE_ERROR             1
#define UNKNOWN_OPCODE          2
#define ROM_DOES_NOT_EXISTS     3

#define MAX_MEMORY_SIZE         (4096)  /* 4 KB */
#define START_MEMORY            (0x200) /* First 512 are reserved */


// Each font is made of 5 8-bit values (1 byte for each row), ranging from 0 to F.
#define FONT_SIZE_BYTES         5
#define FONTS_MEMORY_SIZE       (FONT_SIZE_BYTES * 16)
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
+---+---+---+---+
| 1 | 2 | 3 | C |
+---+---+---+---+
| 4 | 5 | 6 | D |
+---+---+---+---+
| 7 | 8 | 9 | E |
+---+---+---+---+
| A | 0 | B | F |
+---+---+---+---+
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


struct Chip8_state {
    u8 V[16]; // 16 8-bit registers, from V0 to VF.

    u16 I; // Address register.
    u16 stack[16];

    u8 sp; // Stack pointer.
    u16 pc; // Program counter.

    u8 delay_timer; // Delay timer.
    u8 sound_timer; // Sound timer.

    u8 memory[MAX_MEMORY_SIZE];

    u8 screen[SCREEN_SIZE];
};

int get_key_pressed()
{
    for (int i = 0; i < KEY_NUMBER; i++) {
        if (IsKeyDown(input_keys[i])) {
            return chip8_keys[i];
        }
    }

    return -1;
}

void clear_screen(Chip8_state *state)
{
    memset(state->screen, 0, sizeof(state->screen));
}

/* Reference: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#3.1 */
void emulate(Chip8_state *state)
{
    u16 opcode = state->memory[state->pc] << 8 | state->memory[state->pc + 1];
    state->pc += 2;

    // printf("Simulating opcode: %04x\n", opcode);

    
    switch (opcode & 0xF000) {

        case 0x1000: { // 1nnn: Jump to location nnn.
            state->pc = opcode & 0xFFF;
        } break;

        case 0x2000: { // 2nnn: Call subroutine at nnn.
            state->stack[state->sp++] = state->pc;
            state->pc = opcode & 0xFFF;
        } break;

        case 0x3000: { // 3xkk: Skip next instruction if Vx = kk.
            u8 vx = state->V[(opcode & 0xF00) >> 8];
            u8 kk = opcode & 0xFF;
            if (vx == kk) {
                state->pc += 2;
            }
        } break;

        case 0x4000: { // 4xkk: Skip next instruction if Vx != kk.
            u8 vx = state->V[(opcode & 0xF00) >> 8];
            u8 kk = opcode & 0xFF;
            if (vx != kk) {
                state->pc += 2;
            }
        } break;

        case 0x5000: { // 5xy0: Skip next instruction if Vx = Vy.
            u8 vx = state->V[(opcode & 0xF00) >> 8];
            u8 vy = state->V[(opcode & 0xF0) >> 4];
            if (vx == vy) {
                state->pc += 2;
            }
        } break;

        case 0x6000: { // 6xkk: Set Vx = kk.
            state->V[(opcode & 0xF00) >> 8] = (opcode & 0xFF);
        } break;

        case 0x7000: { // 7xkk: Set Vx = Vx + kk.
            state->V[(opcode & 0xF00) >> 8] += (opcode & 0xFF);
        } break;

        case 0x8000: {
            u8 x = (opcode & 0xF00) >> 8;
            u8 y = (opcode & 0xF0) >> 4;

            switch (opcode & 0xF) {
                case 0x0: { // 8xy0: Set Vx = Vy.
                    state->V[x] = state->V[y];
                } break;

                case 0x1: { // 8xy1: Set Vx = Vx OR Vy.
                    state->V[x] |= state->V[y];
                } break;

                case 0x2: { // 8xy2: Set Vx = Vx AND Vy.
                    state->V[x] &= state->V[y];
                } break;

                case 0x3: { // 8xy3: Set Vx = Vx XOR Vy.
                    state->V[x] ^= state->V[y];
                } break;

                case 0x4: { // 8xy4: Set Vx = Vx + Vy, set VF = carry.
                    state->V[x] += state->V[y];

                    state->V[0xF] = (state->V[x] < state->V[y]); // Carry
                } break;

                case 0x5: { // 8xy5: Set Vx = Vx - Vy, set VF = NOT borrow.
                    state->V[0xF] = (state->V[x] >= state->V[y]);

                    state->V[x] -= state->V[y];
                } break;

                case 0x6: { // 8xy6: Set Vx = Vx SHR 1.
                    state->V[0xF] = (state->V[x] & 1); // If least-significant bit is 1
                    
                    state->V[x] >>= 1;
                } break;

                case 0x7: { // 8xy7: Set Vx = Vy - Vx, set VF = NOT borrow.
                    state->V[0xF] = (state->V[y] >= state->V[x]);

                    state->V[x] = state->V[y] - state->V[x];
                } break;

                case 0xE: { // 8xyE: Set Vx = Vx SHL 1.
                    state->V[0xF] = (state->V[x] >> 7); // If most-significant bit is 1

                    state->V[x] <<= 1;
                } break;

            }
        } break;

        case 0x9000: { // 9xy0: Skip next instruction if Vx != Vy.
            u8 vx = state->V[(opcode & 0xF00) >> 8];
            u8 vy = state->V[(opcode & 0xF0) >> 4];
            if (vx != vy) {
                state->pc += 2;
            }
        } break;

        case 0xA000 : { // Annn: Set I = nnn.
            state->I = opcode & 0xFFF;
        } break;

        case 0xB000: { // Bnnn: Jump to location nnn + V0.
            state->pc = (opcode & 0xFFF) + state->V[0];
        } break;

        case 0xC000: { // Cxkk: Set Vx = random byte AND kk.
            u8 random = rand() % 0xFF;
            state->V[(opcode & 0xF00) >> 8] = random & (opcode & 0xFF);
        } break;

        case 0xD000: { // Dxyn: Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
            u8 sprite_width = 8;
            u8 n = opcode & 0xF;
            u8 vx = state->V[(opcode >> 8) & 0xF];
            u8 vy = state->V[(opcode >> 4) & 0xF];

            u8 collision = 0;

            for (int row = 0; row < n; row++) {
                u8 sprite_row = state->memory[state->I + row];; // Each bit is 1 pixel

                for (int col = 0; col < sprite_width; col++) {
                    int screen_index = ((vy + row) * SCREEN_WIDTH) + (vx + col);
                    u8 bit_value = (sprite_row >> (7 - col)) & 1; // Set the corresponding bit to the screen byte
                    
                    if (state->screen[screen_index] == 1 && state->screen[screen_index] & bit_value) {
                        collision = 1;
                    }

                    state->screen[screen_index] ^= bit_value;
                }
            }

            state->V[0xF] = collision;
        } break;
        
        case 0xE000: {
            int key_pressed = get_key_pressed();
            int vx = state->V[(opcode & 0xF00) >> 8];

            switch (opcode & 0xFF) {
                case 0x9E: { // Ex9E: Skip next instruction if key with the value of Vx is pressed.
                    if (key_pressed == vx) {
                        state->pc += 2;
                    }
                } break;

                case 0xA1: { // ExA1: Skip next instruction if key with the value of Vx is not pressed.
                    if (key_pressed != vx) {
                        state->pc += 2;
                    }
                } break;
            }
        } break;
        
        case 0xF000: {
            u8 x = (opcode & 0xF00) >> 8;
            switch (opcode & 0xFF) {
                case 0x07: { // Fx07: Set Vx = delay timer value.
                    state->V[x] = state->delay_timer;
                } break;

                case 0x0A: { // Fx0A: Wait for a key press, store the value of the key in Vx.
                    int key_pressed;
                    while ((key_pressed = get_key_pressed()) == -1)
                        ;

                    state->V[x] = (u8)key_pressed;
                } break;

                case 0x15: { // Fx15: Set delay timer = Vx.
                    state->delay_timer = state->V[x];
                } break;

                case 0x18: { // Fx18: Set sound timer = Vx.
                    state->sound_timer = state->V[x];
                } break;

                case 0x1E: { // Fx1E: Set I = I + Vx.
                    state->I += state->V[x];
                } break;

                case 0x29: { // Fx29: Set I to the memory address of the sprite data corresponding to the hexadecimal digit stored in register VX.
                    state->I = (state->V[x] * FONT_SIZE_BYTES);
                } break;

                case 0x33: { // Fx33: Store BCD representation of Vx in memory locations I, I+1, and I+2.
                             // Takes the decimal value of Vx, and places the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.
                    u8 vx = state->V[x];
                    state->memory[state->I] = vx / 100;
                    vx = vx % 100;
                    state->memory[state->I + 1] = vx / 10;
                    vx = vx % 10;
                    state->memory[state->I + 2] = vx;
                } break;

                case 0x55: { // Fx55: Store the values of registers V0 to VX inclusive in memory starting at address I.
                    for (int i = 0; i <= x; i++) {
                        state->memory[state->I + i] = state->V[i];
                    }
                } break;

                case 0x65: { // Fx65: Fill registers V0 to VX inclusive with the values stored in memory starting at address I.
                    for (int i = 0; i <= x; i++) {
                        state->V[i] = state->memory[state->I + i];
                    }
                } break;
            }
        } break;
        default: {
            switch (opcode & 0xFF) {
                case 0x00EE: { // 00EE: Return from a subroutine.
                    state->pc = state->stack[--state->sp];
                } break;

                case 0x00E0: {
                    clear_screen(state);
                } break;

                default: {
                    fprintf(stderr, "Unknown opcode: %04x\n", opcode);

                    exit(UNKNOWN_OPCODE);
                } break;
            }
        } break;
    }
}

void init_chip8(Chip8_state *state, char *filename_rom)
{
    printf("Loading %s...\n", filename_rom);

    clear_screen(state);

    memset(state->memory, 0, MAX_MEMORY_SIZE);

    // Load font into memory
    for (int i = 0; i < FONTS_MEMORY_SIZE; i++) {
        state->memory[i] = fonts[i];
    }

    FILE *rom;
    errno_t fopen_error = fopen_s(&rom, filename_rom, "rb");
    if (fopen_error != 0) {
        exit(1);
    }
    
    while (!feof(rom)) {
        fread(state->memory + START_MEMORY, 1, MAX_MEMORY_SIZE, rom);
    }

    state->pc = START_MEMORY;
    state->sp = 0;
}


static Chip8_state chip8_state = {};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <game>\n", argv[0]);

        exit(USAGE_ERROR);
    }

    char *filename_rom = argv[1];
    Chip8_state *state = &chip8_state;
    init_chip8(state, filename_rom);
    

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, filename_rom);

    SetTargetFPS(FPS);

    // Main loop
    while (!WindowShouldClose()) {
        if (state->delay_timer > 0) {
            state->delay_timer--;
        }

        if (state->sound_timer > 0) {
            // TODO: play sound
            state->sound_timer--;
        }

        emulate(state);

        BeginDrawing();
            for (int i = 0; i < SCREEN_HEIGHT; i++) {
                for (int j = 0; j < SCREEN_WIDTH; j++) {
                    int index = (i * SCREEN_WIDTH) + j;
                    Color color = (state->screen[index] == 1) ? WHITE : BLACK;
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

    CloseWindow();
    
    return 0;
}
