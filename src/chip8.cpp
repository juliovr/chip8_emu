#include <stdio.h>
#include "../include/raylib.h"

#define SCREEN_WIDTH    800
#define SCREEN_HEIGHT   450
#define FPS             60

int main(int argc, char **argv)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Title");

    SetTargetFPS(FPS);

    // Main loop
    while (!WindowShouldClose()) {

        // Do the work here!

        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText("Congrats! You created your first window!", 190, 200, 20, BLACK);
        EndDrawing();
    }

    CloseWindow();
    
    return 0;
}
