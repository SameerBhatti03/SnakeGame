#include "xparameters.h"
#include "PmodOLED.h"
#include "PmodKYPD.h"
#include "xil_cache.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>



// Define OLED and Keypad objects
PmodOLED oled;
PmodKYPD keypad;

// Custom key mapping for Pmod Keypad
u8 customKeyTable[16] = {
	'1', '2', '3', 'A',
	'4', '5', '6', 'B',
	'7', '8', '9', 'C',
	'*', '0', '#', 'D'};

// Snake parameters
int game_speed_level = 1; // Default to level 1
int game_speeds[] = {200, 100, 50}; // Array for different speed levels (ms delay)
int current_screen = 0; // 0 = Game screen, 1 = High Score screen, 2 = Speed selection

int show_high_score = 0; // 0 = Game screen, 1 = High Score screen
int high_score = 0;
int score = 0;

int snake_x[100] = {5, 4, 3};  // Initial snake positions
int snake_y[100] = {5, 5, 5};
int snake_length = 3;
int food_x = 10;
int food_y = 10;
int game_speed = 200;  // Delay in milliseconds
int snake_dir = 6; 	// Initial direction: RIGHT (6 = Right, 4 = Left, 8 = Down, 2 = Up)

// Display dimensions
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 32;
const int CELL_SIZE = 2;

// Key definitions
const char RESET_KEY = 'D';
const char UP_KEY = '0';
const char DOWN_KEY = '5';
const char LEFT_KEY = '7';
const char RIGHT_KEY = '9';


// Function prototype for draw_pixel
void draw_pixel(PmodOLED *oled, int x, int y);

// Draw pixel function
void draw_pixel(PmodOLED *oled, int x, int y) {
	if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
    	return; // Ignore out-of-bound pixels
	}

	OLED_MoveTo(oled, x, y);   // Move the cursor to (x, y)
	OLED_DrawPixel(oled);  	// Draw a pixel at the cursor position
}

// Function prototypes
void draw_snake();
void draw_food();
void move_snake();
int detect_collision();
void spawn_food();
char read_key();
void reset_game();

void setup() {
	// Initialize OLED
	OLED_Begin(&oled, XPAR_PMODOLED_0_AXI_LITE_GPIO_BASEADDR, XPAR_PMODOLED_0_AXI_LITE_SPI_BASEADDR, 0x00, 0x00);

	// Initialize Keypad
	KYPD_begin(&keypad, XPAR_PMODKYPD_0_AXI_LITE_GPIO_BASEADDR);
	KYPD_loadKeyTable(&keypad, customKeyTable);

	// Clear OLED display
	OLED_ClearBuffer(&oled);
}


void reset_game() {
    if (score > high_score) {
        high_score = score; // Update high score
    }
    score = 0; // Reset the current score
	snake_length = 3;
	snake_x[0] = 5;
	snake_y[0] = 5;
	snake_x[1] = 4;
	snake_y[1] = 5;
	snake_x[2] = 3;
	snake_y[2] = 5;
	//score = 0; //reset score to 0 when snake dies
	spawn_food();
	snake_dir = RIGHT_KEY;
}

void loop() {
    char key = read_key();

    // Handle input

    if (key == RESET_KEY) {
        reset_game();
    } else if (key == UP_KEY && snake_dir != DOWN_KEY) {
        snake_dir = UP_KEY;
    } else if (key == DOWN_KEY && snake_dir != UP_KEY) {
        snake_dir = DOWN_KEY;
    } else if (key == LEFT_KEY && snake_dir != RIGHT_KEY) {
        snake_dir = LEFT_KEY;
    } else if (key == RIGHT_KEY && snake_dir != LEFT_KEY) {
        snake_dir = RIGHT_KEY;
    }


    if (key == 'C') { // Show High Score screen when 'B' is pressed
        show_high_score = 1;
    } else if (key == 'B') { // Show Speed Selection screen when 'C' is pressed
        show_high_score = 2; // Use 2 to indicate speed selection screen
    } else if (key == RESET_KEY) { // Return to the game when reset is pressed
        show_high_score = 0;
        reset_game(); // Reset the game only when returning to play
    }

    if (show_high_score == 1) {
            // Display High Score Screen
            OLED_ClearBuffer(&oled);
            char high_score_display[16];
            sprintf(high_score_display, "High Score: %d", high_score);
            OLED_SetCursor(&oled, 0, 0);
            OLED_DrawString(&oled, high_score_display);
            OLED_Update(&oled);
        } else if (show_high_score == 2) {
            // Display Speed Selection Screen
            OLED_ClearBuffer(&oled);
            OLED_SetCursor(&oled, 0, 0);
            OLED_DrawString(&oled, "Select Speed:");
            OLED_SetCursor(&oled, 0, 1);
            OLED_DrawString(&oled, "1: Normal");
            OLED_SetCursor(&oled, 0, 2);
            OLED_DrawString(&oled, "2: Fast");
            OLED_SetCursor(&oled, 0, 3);
            OLED_DrawString(&oled, "3: Fastest");
            OLED_Update(&oled);

            // Handle speed selection input
            if (key == '1') {
                game_speed_level = 1;
                game_speed = game_speeds[0];
                show_high_score = 0; // Return to game screen
            } else if (key == '2') {
                game_speed_level = 2;
                game_speed = game_speeds[1];
                show_high_score = 0; // Return to game screen
            } else if (key == '3') {
                game_speed_level = 3;
                game_speed = game_speeds[2];
                show_high_score = 0; // Return to game screen
            }
    } else {
        // Regular game logic
        move_snake();
        if (detect_collision()) {
            reset_game();  // Game over
        } else {
            OLED_ClearBuffer(&oled);

            // Add score display
            char score_display[16];
            sprintf(score_display, " %d", score);  // Format the score as a string
            OLED_SetCursor(&oled, 0, 0);           // Set the cursor to the top-left corner
            OLED_DrawString(&oled, score_display); // Display the score string

            // Draw snake and food
            draw_snake();
            draw_food();

            // Update the OLED display
            OLED_Update(&oled);
        }
    }

    // Control game speed
    usleep(game_speed * 1000);
}


void draw_snake() {
	for (int i = 0; i < snake_length; i++) {
    	for (int x = 0; x < CELL_SIZE; x++) {
        	for (int y = 0; y < CELL_SIZE; y++) {
            	draw_pixel(&oled, snake_x[i] * CELL_SIZE + x, snake_y[i] * CELL_SIZE + y);
        	}
    	}
	}
}


void draw_food() {
	for (int x = 0; x < CELL_SIZE; x++) {
    	for (int y = 0; y < CELL_SIZE; y++) {
        	draw_pixel(&oled, food_x * CELL_SIZE + x, food_y * CELL_SIZE + y);
    	}
	}
}




void move_snake() {
	for (int i = snake_length - 1; i > 0; i--) {
    	snake_x[i] = snake_x[i - 1];
    	snake_y[i] = snake_y[i - 1];
	}
	if (snake_dir == UP_KEY) {
    	snake_y[0]--;
	} else if (snake_dir == DOWN_KEY) {
    	snake_y[0]++;
	} else if (snake_dir == LEFT_KEY) {
    	snake_x[0]--;
	} else if (snake_dir == RIGHT_KEY) {
    	snake_x[0]++;
	}

	if (snake_x[0] == food_x && snake_y[0] == food_y) {
    	snake_length++;
    	score++; // Increment Score
    	spawn_food();
	}
}

int detect_collision() {
	if (snake_x[0] < 0 || snake_x[0] >= SCREEN_WIDTH / CELL_SIZE ||
    	snake_y[0] < 0 || snake_y[0] >= SCREEN_HEIGHT / CELL_SIZE) {
    	return 1;
	}
	for (int i = 1; i < snake_length; i++) {
    	if (snake_x[0] == snake_x[i] && snake_y[0] == snake_y[i]) {
        	return 1;
    	}
	}
	return 0;
}

void spawn_food() {
	food_x = rand() % (SCREEN_WIDTH / CELL_SIZE);
	food_y = rand() % (SCREEN_HEIGHT / CELL_SIZE);
}

char read_key() {
	uint16_t key_state = KYPD_getKeyStates(&keypad);
	u8 key = 0;
	if (KYPD_getKeyPressed(&keypad, key_state, &key)) {
    	return key;
	}
	return 0; // Return 0 if no key is pressed
}




int main() {
	setup();
	while (1) {
    	loop();
	}
}
