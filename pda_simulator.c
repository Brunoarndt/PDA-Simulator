#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STATES 100
#define MAX_TRANSITIONS 500
#define MAX_WORDS 100
#define MAX_STACK_SIZE 100
#define MAX_SYMBOL_LENGTH 10

typedef struct {
    char readSymbol;
    char popSymbol;
    char pushSymbol[MAX_SYMBOL_LENGTH];
    int toState;
} Transition;

typedef struct {
    int id;
    int isFinal;
} State;

typedef struct {
    int id;
    Transition transitions[MAX_TRANSITIONS];
    int transitionCount;
} StateNode;

typedef struct {
    StateNode states[MAX_STATES];
    int stateCount;
    int initialState;
    int finalStates[MAX_STATES];
    int finalStateCount;
} Automaton;

typedef struct {
    char items[MAX_STACK_SIZE];
    int top;
} Stack;

void initStack(Stack *stack) {
    stack->top = -1;
}

int isEmpty(Stack *stack) {
    return stack->top == -1;
}

int isFull(Stack *stack) {
    return stack->top == MAX_STACK_SIZE - 1;
}

void push(Stack *stack, char symbol) {
    if (!isFull(stack)) {
        stack->items[++stack->top] = symbol;
    }
}

char pop(Stack *stack) {
    if (!isEmpty(stack)) {
        return stack->items[stack->top--];
    }
    return '\0';
}

char peek(Stack *stack) {
    if (!isEmpty(stack)) {
        return stack->items[stack->top];
    }
    return '\0';
}

void initAutomaton(Automaton *automaton) {
    automaton->stateCount = 0;
    automaton->initialState = -1;
    automaton->finalStateCount = 0;
}

void addState(Automaton *automaton, int id, int isFinal) {
    StateNode state;
    state.id = id;
    state.transitionCount = 0;
    automaton->states[automaton->stateCount++] = state;
    if (isFinal) {
        automaton->finalStates[automaton->finalStateCount++] = id;
    }
}

void addTransition(Automaton *automaton, int fromState, char readSymbol, char popSymbol, char *pushSymbol, int toState) {
    for (int i = 0; i < automaton->stateCount; i++) {
        if (automaton->states[i].id == fromState) {
            Transition transition;
            transition.readSymbol = readSymbol;
            transition.popSymbol = popSymbol;
            strcpy(transition.pushSymbol, pushSymbol);
            transition.toState = toState;
            automaton->states[i].transitions[automaton->states[i].transitionCount++] = transition;
            break;
        }
    }
}

int isFinalState(Automaton *automaton, int state) {
    for (int i = 0; i < automaton->finalStateCount; i++) {
        if (automaton->finalStates[i] == state) {
            return 1;
        }
    }
    return 0;
}

int processWord(Automaton *automaton, char *word) {
    Stack stack;
    initStack(&stack);
    push(&stack, '$'); // Símbolo inicial da pilha

    int currentState = automaton->initialState;
    int wordLen = strlen(word);
    int i = 0;

    while (i <= wordLen) {
        char currentSymbol = (i == wordLen) ? '\0' : word[i];
        char topSymbol = peek(&stack);

        int transitionFound = 0;
        for (int j = 0; j < automaton->states[currentState].transitionCount; j++) {
            Transition t = automaton->states[currentState].transitions[j];
            if (t.readSymbol == currentSymbol && t.popSymbol == topSymbol) {
                pop(&stack);
                for (int k = strlen(t.pushSymbol) - 1; k >= 0; k--) {
                    if (t.pushSymbol[k] != '\0') {
                        push(&stack, t.pushSymbol[k]);
                    }
                }
                currentState = t.toState;
                transitionFound = 1;
                break;
            }
        }

        if (!transitionFound) {
            return 0;
        }

        i++;
    }

    return isFinalState(automaton, currentState) && isEmpty(&stack);
}

void generateDotFile(Automaton *automaton, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Erro ao criar o arquivo DOT.\n");
        return;
    }

    fprintf(file, "digraph PDA {\n");
    fprintf(file, "    rankdir=LR;\n");
    fprintf(file, "    node [shape = circle];\n");

    for (int i = 0; i < automaton->stateCount; i++) {
        if (isFinalState(automaton, automaton->states[i].id)) {
            fprintf(file, "    %d [shape = doublecircle];\n", automaton->states[i].id);
        } else {
            fprintf(file, "    %d;\n", automaton->states[i].id);
        }
    }

    for (int i = 0; i < automaton->stateCount; i++) {
        StateNode state = automaton->states[i];
        for (int j = 0; j < state.transitionCount; j++) {
            Transition t = state.transitions[j];
            char readSymbol = (t.readSymbol == '\0') ? 'E' : t.readSymbol;
            char popSymbol = (t.popSymbol == '\0') ? 'E' : t.popSymbol;
            // Escape characters in labels
            fprintf(file, "    %d -> %d [label = \"%c,%c->%s\"];\n", state.id, t.toState, readSymbol, popSymbol, t.pushSymbol);
        }
    }

    fprintf(file, "}\n");
    fclose(file);
}

void readAutomatonFromFile(Automaton *automaton, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Erro ao abrir o arquivo do autômato.\n");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "initial_state:", 14) == 0) {
            automaton->initialState = atoi(line + 15);
        } else if (strncmp(line, "final_states:", 13) == 0) {
            char *token = strtok(line + 14, ",");
            while (token != NULL) {
                int state = atoi(token);
                addState(automaton, state, 1);
                token = strtok(NULL, ",");
            }
        } else if (strncmp(line, "states:", 7) == 0) {
            char *token = strtok(line + 8, ",");
            while (token != NULL) {
                int state = atoi(token);
                addState(automaton, state, 0);
                token = strtok(NULL, ",");
            }
        } else if (strncmp(line, "transitions:", 12) == 0) {
            while (fgets(line, sizeof(line), file)) {
                int fromState, toState;
                char readSymbol, popSymbol, pushSymbol[MAX_SYMBOL_LENGTH];
                sscanf(line, "%d,%c,%c,%[^,],%d", &fromState, &readSymbol, &popSymbol, pushSymbol, &toState);
                addTransition(automaton, fromState, readSymbol, popSymbol, pushSymbol, toState);
            }
        }
    }

    fclose(file);
}

void readWordsFromFile(char *words[], int *wordCount, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Erro ao abrir o arquivo de palavras.\n");
        return;
    }

    char line[256];
    *wordCount = 0;
    while (fgets(line, sizeof(line), file) && *wordCount < MAX_WORDS) {  // Usando MAX_WORDS aqui
        line[strcspn(line, "\n")] = '\0';  // Remove o caractere de nova linha
        words[*wordCount] = strdup(line); //Armazena o ponteiro da nova string no array words
        (*wordCount)++;
    }

    fclose(file);
}


int main() {
    Automaton automaton;
    initAutomaton(&automaton);

    // Ler autômato do arquivo
    readAutomatonFromFile(&automaton, "automaton.txt");

    // Ler palavras do arquivo
    char *words[MAX_TRANSITIONS];
    int wordCount;
    readWordsFromFile(words, &wordCount, "words.txt");

    // Processar palavras
    for (int i = 0; i < wordCount; i++) {
        int result = processWord(&automaton, words[i]);
        printf("A palavra '%s' é %s pelo autômato.\n", words[i], result ? "aceita" : "rejeitada");
        free(words[i]);  // Liberar memória alocada
    }

    // Gerar o arquivo DOT
    generateDotFile(&automaton, "automaton.dot");
    printf("Arquivo DOT gerado: automaton.dot\n");

    return 0;
}
