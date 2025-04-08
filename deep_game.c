#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <ncurses.h>

// Game configuration
#define GRID_WIDTH 5
#define GRID_HEIGHT 5
#define NUM_ENTITIES 3

// gcc -o game deep_game.c -lncurses -lm
// ./game

// Entity names for display
const char* ENTITY_NAMES[NUM_ENTITIES] = {"Hare", "Fox", "Rock"};

// Ecosystem parameters (now including movement probabilities)
const double GROWTH_RATES[NUM_ENTITIES] = {1.4, -0.4, 0};
const double MOVE_PROBABILITIES[NUM_ENTITIES] = {0.3, 0.2, 0}; // Species-specific movement
const double INTERACTIONS[NUM_ENTITIES][NUM_ENTITIES] = {
    {-0.002, -0.002, 0},   // Hare
    {0.001, -0.004, 0},    // Fox
    {0, 0, 0}    // Rock
};

// Game state
int ecosystem[GRID_HEIGHT][GRID_WIDTH][NUM_ENTITIES];
int cursor_x = 0, cursor_y = 0;
int selected_entity = 0;
int turn_count = 0;

// Initialize the ecosystem with random populations
void initialize_ecosystem() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            ecosystem[y][x][0] = 50 + rand() % 50;  // Plants
            ecosystem[y][x][1] = 10 + rand() % 20;  // Herbivores
            ecosystem[y][x][2] = 5 + rand() % 10;    // Carnivores
        }
    }
}

// Apply Lotka-Volterra interactions to all cells
void apply_ecosystem_interactions() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            double current[NUM_ENTITIES], change[NUM_ENTITIES], new_pop[NUM_ENTITIES];
            
            // Get current populations
            for (int e = 0; e < NUM_ENTITIES; e++) {
                current[e] = ecosystem[y][x][e];
            }
            
            // Calculate interactions
            for (int e = 0; e < NUM_ENTITIES; e++) {
                change[e] = 0;
                for (int f = 0; f < NUM_ENTITIES; f++) {
                    change[e] += INTERACTIONS[e][f] * current[f];
                }
            }
            
            // Apply changes
            for (int e = 0; e < NUM_ENTITIES; e++) {
                new_pop[e] = current[e] + current[e] * (GROWTH_RATES[e] + change[e]);
                ecosystem[y][x][e] = (int)fmax(0, floor(new_pop[e]));
            }
        }
    }
}

// Simulate movement between cells with species-specific probabilities
void simulate_movement() {
    int new_populations[GRID_HEIGHT][GRID_WIDTH][NUM_ENTITIES] = {0};
    
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            for (int e = 0; e < NUM_ENTITIES; e++) {
                // Use species-specific movement probability
                double move_prob = MOVE_PROBABILITIES[e];
                int moving = (rand() / (double)RAND_MAX < move_prob) ? 
                            ecosystem[y][x][e] / 2 : 0;
                int per_neighbor = moving / 8;
                
                // Distribute to neighbors
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        
                        int nx = x + dx;
                        int ny = y + dy;
                        
                        if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT) {
                            new_populations[ny][nx][e] += per_neighbor;
                            ecosystem[y][x][e] -= per_neighbor;
                        }
                    }
                }
            }
        }
    }
    
    // Apply movement results
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            for (int e = 0; e < NUM_ENTITIES; e++) {
                ecosystem[y][x][e] += new_populations[y][x][e];
            }
        }
    }
}

// Draw the game interface
void draw_interface() {
    clear();
    
    // Header information
    printw("Ecosystem Simulation - Turn %d\n", turn_count);
    printw("Selected: %s | Controls: Arrows=Move, +/-=Adjust, Tab=Switch, Enter=Next Turn, q=Quit\n", 
           ENTITY_NAMES[selected_entity]);
    
    // Draw the grid
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (x == cursor_x && y == cursor_y) {
                attron(A_REVERSE);
            }
            
            printw("[");
            for (int e = 0; e < NUM_ENTITIES; e++) {
                printw("%s%3d", e == 0 ? "" : ",", ecosystem[y][x][e]);
            }
            printw("] ");
            
            if (x == cursor_x && y == cursor_y) {
                attroff(A_REVERSE);
            }
        }
        printw("\n");
    }
    
    // Footer with current entity stats
    printw("\nCurrent Entity: %s\n", ENTITY_NAMES[selected_entity]);
    printw("Growth rate: %.2f | Move prob: %.2f\n", 
           GROWTH_RATES[selected_entity], MOVE_PROBABILITIES[selected_entity]);
    printw("Interactions: ");
    for (int e = 0; e < NUM_ENTITIES; e++) {
        printw("%s: %.3f ", ENTITY_NAMES[e], INTERACTIONS[selected_entity][e]);
    }
    
    refresh();
}

int main() {
    // Initialize game
    srand(time(NULL));
    initialize_ecosystem();
    
    // Set up ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    // Game loop
    int input;
    do {
        draw_interface();
        input = getch();
        
        switch (input) {
            case KEY_UP:    if (cursor_y > 0) cursor_y--; break;
            case KEY_DOWN:  if (cursor_y < GRID_HEIGHT-1) cursor_y++; break;
            case KEY_LEFT:  if (cursor_x > 0) cursor_x--; break;
            case KEY_RIGHT: if (cursor_x < GRID_WIDTH-1) cursor_x++; break;
            case '+': 
                ecosystem[cursor_y][cursor_x][selected_entity]++;
                break;
            case '-':
                if (ecosystem[cursor_y][cursor_x][selected_entity] > 0) {
                    ecosystem[cursor_y][cursor_x][selected_entity]--;
                }
                break;
            case '\t':  // Tab to switch entities
                selected_entity = (selected_entity + 1) % NUM_ENTITIES;
                break;
            case '\n':  // Enter to advance turn
                apply_ecosystem_interactions();
                simulate_movement();
                turn_count++;
                break;
        }
    } while (input != 'q');
    
    endwin();
    return 0;
}