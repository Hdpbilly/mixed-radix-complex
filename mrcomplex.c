#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 256
#define MEMORY_SIZE 1024
#define SYMBOL_RANGE 256  // For all ASCII characters
#define MAX_PLACES 1000    // Maximum places in number lines for simplicity

typedef struct {
    int values[MAX_PLACES];
    int last_position; // Last position where the symbol was encountered
    int count;         // Number of values in the number line
} NumberLine;

typedef struct {
    char symbols[BUFFER_SIZE];
    int head;
    int tail;
    int count; // Track number of elements in the buffer
} BufferSpace;

typedef struct {
    NumberLine number_lines[SYMBOL_RANGE]; // Number lines for all possible symbols
    int current_step;                      // Current encoding step
    int symbol_order[SYMBOL_RANGE];        // Order in which symbols are first encountered
    int order_count;                       // Count of unique symbols encountered
} MemorySpace;

typedef struct {
    BufferSpace *buffer;
    MemorySpace *memory;
} Encoder;

// Initialize buffer space
void initialize_buffer_space(BufferSpace *buffer) {
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
    memset(buffer->symbols, 0, BUFFER_SIZE);
}

// Initialize memory space
void initialize_memory_space(MemorySpace *memory) {
    memory->current_step = 0;
    memory->order_count = 0;
    for (int i = 0; i < SYMBOL_RANGE; i++) {
        memory->number_lines[i].last_position = -1; // Not encountered yet
        memory->number_lines[i].count = 0;
        memset(memory->number_lines[i].values, 0, MAX_PLACES * sizeof(int));
        memory->symbol_order[i] = -1; // Initialize symbol order
    }
}

// Allocate null memory when buffer is empty
void allocate_null_memory(MemorySpace *memory) {
    memory->current_step++;
    for (int i = 0; i < SYMBOL_RANGE; i++) {
        if (memory->number_lines[i].count > 0) {
            memory->number_lines[i].values[memory->number_lines[i].count++] = 0;
        }
    }
}

// Add symbol to buffer space
void add_symbol_to_buffer(BufferSpace *buffer, char symbol) {
    if (buffer->count < BUFFER_SIZE) {
        buffer->symbols[buffer->tail++] = symbol;
        buffer->tail %= BUFFER_SIZE;
        buffer->count++;
    } else {
        printf("Buffer is full. Cannot add symbol.\n");
    }
}

// Remove symbol from buffer space
char remove_symbol_from_buffer(BufferSpace *buffer) {
    if (buffer->count > 0) {
        char symbol = buffer->symbols[buffer->head++];
        buffer->head %= BUFFER_SIZE;
        buffer->count--;
        return symbol;
    } else {
        return '\0'; // Buffer is empty
    }
}

// Update memory space with symbol and calculate radix
void update_memory_space(Encoder *encoder, char symbol) {
    MemorySpace *memory = encoder->memory;
    int symbol_index = (int)symbol;
    NumberLine *number_line = &memory->number_lines[symbol_index];

    if (number_line->last_position == -1) {
        // First encounter of the symbol
        number_line->last_position = memory->current_step;
        number_line->values[number_line->count++] = 0; // Initialize number line with 0
        memory->symbol_order[memory->order_count++] = symbol_index; // Track the order of first encounter
    } else {
        // Repeated symbol
        int radix = memory->current_step - number_line->last_position;
        number_line->last_position = memory->current_step;
        number_line->values[number_line->count++] = radix;
    }

    // Update all number lines for this step
    for (int i = 0; i < SYMBOL_RANGE; i++) {
        if (memory->number_lines[i].count > 0 && i != symbol_index) {
            memory->number_lines[i].values[memory->number_lines[i].count++] = 0;
        }
    }

    memory->current_step++;
}

// Process symbols from buffer space to memory space
void process_symbols(Encoder *encoder) {
    while (encoder->buffer->count > 0) {
        char symbol = remove_symbol_from_buffer(encoder->buffer);
        if (symbol != '\0') {
            update_memory_space(encoder, symbol);
        }
    }
}

// Print mixed radix form
void print_mixed_radix_form(MemorySpace *memory) {
    for (int i = 0; i < memory->order_count; i++) {
        int symbol_index = memory->symbol_order[i];
        if (memory->number_lines[symbol_index].count > 0) {
            printf("%c: ", symbol_index);
            int printed_any = 0;
            for (int j = 1; j < memory->number_lines[symbol_index].count; j++) {
                if (memory->number_lines[symbol_index].values[j] != 0) {
                    if (printed_any) {
                        printf(",");
                    }
                    printf("%d", memory->number_lines[symbol_index].values[j]);
                    printed_any = 1;
                }
            }
            if (!printed_any) {
                printf("incomplete");
            }
            printf("\n");
        }
    }
}

// Test case
void run_test_case(const char *input_string) {
    BufferSpace buffer;
    MemorySpace memory;
    Encoder encoder = {&buffer, &memory};

    initialize_buffer_space(&buffer);
    initialize_memory_space(&memory);

    // Adding symbols to the buffer
    for (int i = 0; i < strlen(input_string); i++) {
        add_symbol_to_buffer(&buffer, input_string[i]);
    }

    process_symbols(&encoder);

    // Print number lines in the order of their first occurrence
    for (int i = 0; i < memory.order_count; i++) {
        int symbol_index = memory.symbol_order[i];
        if (memory.number_lines[symbol_index].count > 0) {
            printf("Number line for symbol '%c': ", symbol_index);
            for (int j = 0; j < memory.number_lines[symbol_index].count; j++) {
                printf("%d ", memory.number_lines[symbol_index].values[j]);
            }
            printf("\n");
        }
    }

    // Print the mixed radix form
    printf("\nMixed radix form:\n");
    print_mixed_radix_form(&memory);
}

int main() {
    char input_string[BUFFER_SIZE];
    printf("Input string to be encoded: ");
    fgets(input_string, BUFFER_SIZE, stdin);
    input_string[strcspn(input_string, "\n")] = 0; // Remove trailing newline

    run_test_case(input_string);
    return 0;
}
