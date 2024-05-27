#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <pdcurses.h>

#define BUFFER_SIZE 256
#define MEMORY_SIZE 1024
#define SYMBOL_RANGE 256
#define MAX_PLACES 2000

typedef struct {
    int values[MAX_PLACES];
    int last_position;
    int count;
} NumberLine;

typedef struct {
    char symbols[BUFFER_SIZE];
    int head;
    int tail;
    int count;
} BufferSpace;

typedef struct {
    NumberLine number_lines[SYMBOL_RANGE];
    int current_step;
    int symbol_order[SYMBOL_RANGE];
    int order_count;
} MemorySpace;

typedef struct {
    BufferSpace *buffer;
    MemorySpace *memory;
    int allow_zero_streaming;
    int update_speed;
} Encoder;

void initialize_buffer_space(BufferSpace *buffer) {
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
    memset(buffer->symbols, 0, BUFFER_SIZE);
}

void initialize_memory_space(MemorySpace *memory) {
    memory->current_step = 0;
    memory->order_count = 0;
    for (int i = 0; i < SYMBOL_RANGE; i++) {
        memory->number_lines[i].last_position = -1;
        memory->number_lines[i].count = 0;
        memset(memory->number_lines[i].values, -1, MAX_PLACES * sizeof(int));
        memory->symbol_order[i] = -1;
    }
}

void add_symbol_to_buffer(BufferSpace *buffer, char symbol) {
    if (buffer->count < BUFFER_SIZE) {
        buffer->symbols[buffer->tail++] = tolower(symbol);
        buffer->tail %= BUFFER_SIZE;
        buffer->count++;
    } else {
        printf("Buffer is full. Cannot add symbol.\n");
    }
}

char remove_symbol_from_buffer(BufferSpace *buffer) {
    if (buffer->count > 0) {
        char symbol = buffer->symbols[buffer->head++];
        buffer->head %= BUFFER_SIZE;
        buffer->count--;
        return symbol;
    } else {
        return '\0';
    }
}

void update_memory_space(Encoder *encoder, char symbol) {
    MemorySpace *memory = encoder->memory;
    int symbol_index = (int)symbol;
    NumberLine *number_line = &memory->number_lines[symbol_index];

    if (number_line->last_position == -1) {
        number_line->last_position = memory->current_step;
        number_line->values[number_line->count++] = 0;
        memory->symbol_order[memory->order_count++] = symbol_index;
    } else {
        int radix = memory->current_step - number_line->last_position;
        number_line->last_position = memory->current_step;
        number_line->values[number_line->count++] = radix;
    }

    for (int i = 0; i < SYMBOL_RANGE; i++) {
        if (memory->number_lines[i].count > 0 && i != symbol_index) {
            if (encoder->allow_zero_streaming) {
                memory->number_lines[i].values[memory->number_lines[i].count++] = -1;
            }
        }
    }

    memory->current_step++;
}

char *convert_to_base36(int value) {
    static char base36[3];
    const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";

    if (value < 36) {
        base36[0] = digits[value];
        base36[1] = '\0';
    } else {
        base36[0] = digits[value / 36];
        base36[1] = digits[value % 36];
        base36[2] = '\0';
    }

    return base36;
}

void print_mixed_radix_form(MemorySpace *memory, int allow_zero_streaming) {
    clear();

    int cols = COLS;
    int rows = LINES;

    for (int i = 0; i < memory->order_count; i++) {
        int symbol_index = memory->symbol_order[memory->order_count - i - 1];
        mvaddch(0, i * 4, symbol_index); // Adding 3 character space between symbols
    }

    // Draw a horizontal line below the symbols
    mvhline(1, 0, '-', cols);

    for (int i = 0; i < memory->order_count; i++) {
        int symbol_index = memory->symbol_order[memory->order_count - i - 1];

        for (int j = 0; j < rows - 2 && j < memory->number_lines[symbol_index].count; j++) {
            int value = memory->number_lines[symbol_index].values[memory->number_lines[symbol_index].count - j - 1];
            if (value != -1 || allow_zero_streaming) {
                char *base36 = convert_to_base36(value != -1 ? value : 0);
                mvprintw(j + 2, i * 4, "%s", base36);
            } else {
                break; // Stop if we encounter a -1 and zero streaming is not allowed
            }
        }
    }

    refresh();
}

void process_symbols(Encoder *encoder) {
    while (encoder->buffer->count > 0) {
        char symbol = remove_symbol_from_buffer(encoder->buffer);
        if (symbol != '\0') {
            update_memory_space(encoder, symbol);
        }

        print_mixed_radix_form(encoder->memory, encoder->allow_zero_streaming);
        usleep(encoder->update_speed * 1000); // Update speed in milliseconds
    }
}

void read_file_and_encode(const char *file_path, Encoder *encoder) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        printf("Failed to open file.\n");
        return;
    }

    char ch;
    while ((ch = fgetc(file)) != EOF) {
        add_symbol_to_buffer(encoder->buffer, ch);
        process_symbols(encoder);
    }

    fclose(file);
}

int main() {
    char file_path[BUFFER_SIZE];
    printf("Enter the path to the text file: ");
    fgets(file_path, BUFFER_SIZE, stdin);
    file_path[strcspn(file_path, "\n")] = 0;

    BufferSpace buffer;
    MemorySpace memory;
    Encoder encoder = {&buffer, &memory, 0, 100};

    initialize_buffer_space(&buffer);
    initialize_memory_space(&memory);

    initscr();
    noecho();
    cbreak();
    curs_set(FALSE);
    keypad(stdscr, TRUE); // Enable keypad for capturing resize events

    // Set initial window size
    int initial_width = 400;  // Adjust this width as needed
    int initial_height = 24;  // Adjust this height as needed
    resize_term(initial_height, initial_width);

    printf("Allow 0 streaming? (y/n): ");
    char response;
    response = getchar();
    encoder.allow_zero_streaming = (response == 'y' || response == 'Y') ? 1 : 0;
    while (getchar() != '\n'); // Clear input buffer

    printf("Enter update speed in milliseconds: ");
    scanf("%d", &encoder.update_speed);
    while (getchar() != '\n'); // Clear input buffer

    printf("Press Enter to start encoding...");
    getchar();

    read_file_and_encode(file_path, &encoder);

    while (1) {
        int ch = getch();
        if (ch == KEY_RESIZE) {
            // Handle terminal resize event
            if (is_termresized()) {
                int new_height, new_width;
                getmaxyx(stdscr, new_height, new_width);
                resize_term(new_height, new_width);
                print_mixed_radix_form(encoder.memory, encoder.allow_zero_streaming);
            }
        }
        // Add additional input handling if necessary
    }

    endwin();

    return 0;
