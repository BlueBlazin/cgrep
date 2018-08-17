#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*************************************************************************
 * Directed Graph
**************************************************************************/


typedef struct List {
    size_t size;
    int *values;
} List;

typedef struct Graph {
    size_t size;    /* number of nodes in graph */
    char *values;   /* value at each node */
    List **adj;     /* adj list */
} Graph;


static void init_lists(Graph *g, size_t size) {
    size_t i;
    for (i = 0; i < size; i++) {
        g->adj[i] = malloc(sizeof(List));
        g->adj[i]->size = 0;
        g->adj[i]->values = NULL;
    }
}

Graph *init_graph(size_t size) {
    Graph *g = malloc(sizeof(Graph));
    g->size = size;
    g->values = malloc(size);
    g->adj = malloc(size * sizeof(List *));
    init_lists(g, size);
    return g;
}

void add_edge(Graph *g, int u, int v) {
    g->adj[u]->size++;
    size_t size = g->adj[u]->size;
    g->adj[u]->values = realloc(g->adj[u]->values, size * sizeof(int));
    g->adj[u]->values[size - 1] = v;
}


/*************************************************************************
 * Stack
**************************************************************************/

typedef struct Node {
    int value;
    struct Node *prev;
} Node;


void push(Node **top, int value) {
    Node *node = (Node *) malloc(sizeof(Node));
    node->value = value;
    node->prev = *top;
    *top = node;
}

int pop(Node **top) {
    int val = (*top)->value;
    Node *new_top = (*top)->prev;
    free(*top);
    *top = new_top;
    return val;
}

int is_empty(Node *top) {
    return top == NULL;
}

void destroy_stack(Node *top) {
    while (top != NULL) {
        pop(&top);
    }
}


void print_stack(Node *stack);


/*************************************************************************
 * Regex to NFA
**************************************************************************/

static void set_graph_vals_from_string(Graph *g, const char *str) {
    size_t size = g->size;
    size_t i;
    for (i = 0; i < size; i++) {
        g->values[i] = str[i];
    }
}

static void build_transitions(Graph *nfa, Node **ops) {
    char *re = nfa->values;
    size_t size = nfa->size;
    size_t i;
    for (i = 0; i < size - 1; i++) {
        int lp = i;
    
        if (re[i] == '(' || re[i] == '|') push(ops, i);
        else if (re[i] == ')') {
            int or = pop(ops);
            if (re[or] == '|') {
                lp = pop(ops);
                add_edge(nfa, lp, or+1);
                add_edge(nfa, or, i);
            }
            else lp = or;
        }

        if (i < size - 1 && re[i+1] == '*') {
            add_edge(nfa, lp, i + 1);
            add_edge(nfa, i + 1, lp);
        }

        if (i < size - 1 && re[i+1] == '?') {
            add_edge(nfa, lp, i + 1);
        }

        if (re[i] == '(' || re[i] == '*' || re[i] == ')' || re[i] == '?') {
            add_edge(nfa, i, i+1);
        }
    }
}

Graph *build_nfa(const char *regex) {
    size_t size = strlen(regex);
    Graph *nfa = init_graph(size + 1);
    Node *stack = NULL;

    set_graph_vals_from_string(nfa, regex);
    build_transitions(nfa, &stack);

    destroy_stack(stack);
    return nfa;
}


/*************************************************************************
 * NFA Simulation
**************************************************************************/

static void init_bool_buffer(bool buf[], size_t size, bool val) {
    size_t i;
    for (i = 0; i < size; i++) buf[i] = val;
}

static void 
add_state(int s, Node **stack, Graph *nfa, bool already_on[]) {
    push(stack, s);
    already_on[s] = true;
    List *adj_list = nfa->adj[s];
    size_t i;
    for (i = 0; i < adj_list->size; i++) {
        int t = adj_list->values[i];
        if (!already_on[t]) add_state(t, stack, nfa, already_on);
    }
}

static void 
make_next_moves(Graph *nfa, char c, Node **old_states, Node **new_states, bool already_on[]) {
    while (!is_empty(*old_states)) {
        int s = pop(old_states);
        if (nfa->values[s] == c || nfa->values[s] == '.') {
            if (!already_on[s+1]) add_state(s + 1, new_states, nfa, already_on);
        }
    }

    while (!is_empty(*new_states)) {
        int s = pop(new_states);
        push(old_states, s);
        already_on[s] = false;
    }
}

void epsilon_closure(Graph *nfa, Node **stack) {
    bool already_on[nfa->size];
    size_t i;
    for (i = 0; i < nfa->size; i++) already_on[i] = false;
    add_state(0, stack, nfa, already_on);
}

bool match(Graph *nfa, const char *text) {
    size_t size = nfa->size;
    Node *old_states = NULL;
    Node *new_states = NULL;
    bool already_on[size];

    init_bool_buffer(already_on, size, false);

    epsilon_closure(nfa, &old_states);

    size_t i;
    for (i = 0; i < strlen(text); i++) {
        make_next_moves(nfa, text[i], &old_states, &new_states, already_on);
    }

    int s;
    int end = (int) size - 1;
    while (!is_empty(old_states)) {
        s = pop(&old_states);
        if (s == end) return true;
    }

    return false;
}


/*************************************************************************
 * Main
**************************************************************************/


int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: ./cgrep <pattern> <text>\n");
        exit(1);
    }
    const char *pattern = argv[1];
    const char *text = argv[2];

    Graph *nfa = build_nfa(pattern);
    bool res = match(nfa, text);
    printf("result: %s\n", (res == true ? "true" : "false"));
    return 0;
}
