#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "address_map_arm.h"

#define WIDTH 40
#define HEIGHT 20
#define PACMAN 'C'
#define WALL '#'
#define FOOD '.'
#define EMPTY ' '
#define DEMON 'X'
#define KEY_BASE 0xFF200050
#define SW_BASE 0xFF200040
#define GAME_DELAY 500000

#define PIXEL_BUF_CTRL_BASE 0xFF203020
#define FPGA_PIXEL_BASE     0x08000000

#define MAX_DEMONS 4

volatile short *pixel_buffer = (short *)FPGA_PIXEL_BASE;


typedef struct {
    int x, y;
    char under;
} Demon;

Demon demons[MAX_DEMONS];


int pacman_x, pacman_y, score = 0, food = 0, curr = 0, res = 0;
char board[HEIGHT][WIDTH];

volatile char *character_buffer = (char *)FPGA_CHAR_BASE;

volatile int direction = 0;


const char fixed_map[HEIGHT][WIDTH + 1] = {
    "########################################",
    "#..............##..............##......#",
    "#.####.#####.####.####.#####.####.####.#",
    "#.####.#####.####.####.#####.####.####.#",
    "#......................................#",
    "#.####.##.##########.##.##########.##..#",
    "#......................................#",
    "#.####.####.####.####.####.####.########",
    "#.####.####.####.####.####.####.########",
    "#......................................#",
    "####.####.##########.##.##########.#####",
    "#......................................#",
    "#.####.#####.####.####.#####.####.####.#",
    "#......................................#",
    "#.####.#####.####.####.#####.####.####.#",
    "#......................................#",
    "#.##########.##.##########.##.########.#",
    "#......................................#",
    "########################################"
};


void video_text(int x, int y, char *text) {
    int offset = (y << 7) + x;
    while (*text) {
        *(character_buffer + offset) = *text++;
        offset++;
    }
}


void draw_board() {
    char line[WIDTH + 1];
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            line[j] = board[i][j];
        }
        line[WIDTH] = '\0';
        video_text(0, i, line); 
    }

    char score_str[40];
    sprintf(score_str, "Score: %d  Eaten: %d", score, curr);
    video_text(0, HEIGHT + 1, score_str);
}

/*
void initialize() {
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            char ch = fixed_map[i][j];
            board[i][j] = ch;
            if (ch == FOOD)
                food++;
        }
    }

    // Set Pac-Man's initial position (e.g., near center)
    pacman_x = WIDTH / 2;
    pacman_y = HEIGHT / 2;
    board[pacman_y][pacman_x] = PACMAN;
}*/
void initialize() {
    food = 0;
    score = 0;
    curr = 0;
    res = 0;

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            char ch = fixed_map[i][j];
            board[i][j] = ch;
            if (ch == FOOD)
                food++;
        }
    }

    // Set Pac-Man's initial position
    pacman_x = WIDTH / 2;
    pacman_y = HEIGHT / 2;
    board[pacman_y][pacman_x] = PACMAN;

    // Place demons in corners or strategic spots
    demons[0] = (Demon){1, 1};
    demons[1] = (Demon){WIDTH - 2, 1};
    demons[2] = (Demon){1, HEIGHT - 3};
    demons[3] = (Demon){WIDTH - 2, HEIGHT - 3};

    demons[0].under = FOOD;
    demons[1].under = FOOD;
    demons[2].under = FOOD;
    demons[3].under = FOOD;

    for (int i = 0; i < MAX_DEMONS; i++) {
        board[demons[i].y][demons[i].x] = DEMON;
    }
}


void move(int dx, int dy) {
    int x = pacman_x + dx;
    int y = pacman_y + dy;

    if (board[y][x] == WALL) return;

    if (board[y][x] == FOOD) {
        score++;
        food--;
        curr++;
        if (food == 0) {
            res = 2;
            return;
        }
    } else if (board[y][x] == DEMON) {
        res = 1;
        return;
    }

    board[pacman_y][pacman_x] = EMPTY;
    pacman_x = x;
    pacman_y = y;
    board[pacman_y][pacman_x] = PACMAN;
}



void move_demons() {
    for (int i = 0; i < MAX_DEMONS; i++) {
        int dx = 0, dy = 0;
        int diff_x = pacman_x - demons[i].x;
        int diff_y = pacman_y - demons[i].y;

        if (abs(diff_x) > 0 && abs(diff_y) > 0) {
            if (rand() % 2 == 0)
                dx = (diff_x > 0) ? 1 : -1;
            else
                dy = (diff_y > 0) ? 1 : -1;
        } else if (abs(diff_x) > 0) {
            dx = (diff_x > 0) ? 1 : -1;
        } else if (abs(diff_y) > 0) {
            dy = (diff_y > 0) ? 1 : -1;
        }

        int new_x = demons[i].x + dx;
        int new_y = demons[i].y + dy;

        if (board[new_y][new_x] == EMPTY || board[new_y][new_x] == FOOD) {
            board[demons[i].y][demons[i].x] = demons[i].under;

            demons[i].under = board[new_y][new_x];

            demons[i].x = new_x;
            demons[i].y = new_y;
            board[new_y][new_x] = DEMON;
        }

        if (new_x == pacman_x && new_y == pacman_y) {
            res = 1; // Game over
        }
    }
}




int game(void){
    initialize();

    draw_board();
    video_text(0, HEIGHT + 3, "Press Key[3] up, Key[2] Left, Key[1] Down, Key[0] Right to move. Game auto-runs. Switch SW[0] to start/stop.");

    volatile int *KEY_ptr = (int *)SW_BASE;

    while (1) {
        draw_board();

        for (volatile int d = 0; d < GAME_DELAY; d++) ;

        volatile int *KEY_ptr_button = (int *)KEY_BASE;
        int key_val = *KEY_ptr_button;

        if ((key_val & 0x8))       // KEY[3] => Up
            direction = 1;
        else if ((key_val & 0x4))  // KEY[2] => Left
            direction = 2;
        else if ((key_val & 0x2))  // KEY[1] => Down
            direction = 3;
        else if ((key_val & 0x1))  // KEY[0] => Right
            direction = 4;

        switch (direction) {
            case 1: move(0, -1); break;   // Up (KEY[3])
            case 2: move(-1, 0); break;   // Left (KEY[2])
            case 3: move(0, 1); break;    // Down (KEY[1])
            case 4: move(1, 0); break;    // Right (KEY[0])
            default: break;
        }

        move_demons();

        if (res == 1) {
            video_text(0, HEIGHT + 5, "Game Over! Hit by Demon.");
            break;
        } else if (res == 2) {
            video_text(0, HEIGHT + 5, "You Win! All food collected.");
            break;
        }

        for (volatile int d = 0; d < 200000; d++) ;

        if ((*KEY_ptr & 0x1) == 0) break;
    }

    return 0;
    
}

int main(void) {
    
    while (1) {
        volatile int *KEY_ptr = (int *)SW_BASE;
        if ((*KEY_ptr & 0x1) == 1){
            pacman_x, pacman_y, score = 0, food = 0, curr = 0, res = 0;
            char limpa[60];
            sprintf(limpa, "                                                            ");
            for(int i = 0; i<60; i++){
                video_text(0, HEIGHT + i, limpa);
            }
            game();
        }
    }

    return 0;
}
