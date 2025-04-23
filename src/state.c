#include "state.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "snake_utils.h"

// Definiciones de funciones de ayuda.
static void set_board_at(game_state_t* state, unsigned int row, unsigned int col, char ch);
static bool is_tail(char c);
static bool is_head(char c);
static bool is_snake(char c);
static char body_to_tail(char c);
static char head_to_body(char c);
static unsigned int get_next_row(unsigned int cur_row, char c);
static unsigned int get_next_col(unsigned int cur_col, char c);
static void find_head(game_state_t* state, unsigned int snum);
static char next_square(game_state_t* state, unsigned int snum);
static void update_tail(game_state_t* state, unsigned int snum);
static void update_head(game_state_t* state, unsigned int snum);

/* Tarea 1 */
game_state_t* create_default_state() {
  game_state_t* state = malloc(sizeof(game_state_t));
  if (!state) return NULL;

  state->num_rows = 18;
  unsigned int cols = 20;

  state->board = malloc(sizeof(char*) * state->num_rows);
  for (unsigned int i = 0; i < state->num_rows; ++i) {
    state->board[i] = malloc(sizeof(char) * cols);
    for (unsigned int j = 0; j < cols; ++j) {
      state->board[i][j] = ' ';
    }
  }

  state->board[2][9] = '*';
  state->board[2][2] = 'd';
  state->board[2][3] = '-';
  state->board[2][4] = 'D';

  state->num_snakes = 1;
  state->snakes = malloc(sizeof(snake_t));
  state->snakes[0].tail_row = 2;
  state->snakes[0].tail_col = 2;
  state->snakes[0].head_row = 2;
  state->snakes[0].head_col = 4;
  state->snakes[0].live = true;

  return state;
}

/* Tarea 2 */
void free_state(game_state_t* state) {
  if (!state) return;

  for (unsigned int i = 0; i < state->num_rows; ++i) {
    free(state->board[i]);
  }

  free(state->board);

  free(state->snakes);

  free(state);
}



/* Tarea 3 */
void print_board(game_state_t* state, FILE* fp) {
  for (unsigned int i = 0; i < state->num_rows; ++i) {
    for (unsigned int j = 0; state->board[i][j] != '\0'; ++j) {
      fputc(state->board[i][j], fp);
    }
    fputc('\n', fp);
  }
}



/**
 * Guarda el estado actual a un archivo. No modifica el objeto/struct state.
 * (ya implementada para que la utilicen)
*/
void save_board(game_state_t* state, char* filename) {
  FILE* f = fopen(filename, "w");
  print_board(state, f);
  fclose(f);
}

/* Tarea 4.1 */


/**
 * Funcion de ayuda que obtiene un caracter del tablero dado una fila y columna
 * (ya implementado para ustedes).
*/
char get_board_at(game_state_t* state, unsigned int row, unsigned int col) {
  return state->board[row][col];
}


/**
 * Funcion de ayuda que actualiza un caracter del tablero dado una fila, columna y
 * un caracter.
 * (ya implementado para ustedes).
*/
static void set_board_at(game_state_t* state, unsigned int row, unsigned int col, char ch) {
  state->board[row][col] = ch;
}


/**
 * Retorna true si la variable c es parte de la cola de una snake.
 * La cola de una snake consiste de los caracteres: "wasd"
 * Retorna false de lo contrario.
*/
static bool is_tail(char c) {
  return c == 'w' || c == 'a' || c == 's' || c == 'd';
}



/**
 * Retorna true si la variable c es parte de la cabeza de una snake.
 * La cabeza de una snake consiste de los caracteres: "WASDx"
 * Retorna false de lo contrario.
*/
static bool is_head(char c) {
  return c == 'W' || c == 'A' || c == 'S' || c == 'D' || c == 'x';
}


/**
 * Retorna true si la variable c es parte de una snake.
 * Una snake consiste de los siguientes caracteres: "wasd^<v>WASDx"
*/
static bool is_snake(char c) {
  return is_tail(c) || is_head(c) ||
         c == '^' || c == '<' || c == 'v' || c == '>';
}



/**
 * Convierte un caracter del cuerpo de una snake ("^<v>")
 * al caracter que correspondiente de la cola de una
 * snake ("wasd").
*/
static char body_to_tail(char c) {
  switch (c) {
    case '^': return 'w';
    case 'v': return 's';
    case '<': return 'a';
    case '>': return 'd';
    default:  return '?';
  }
}



/**
 * Convierte un caracter de la cabeza de una snake ("WASD")
 * al caracter correspondiente del cuerpo de una snake
 * ("^<v>").
*/
static char head_to_body(char c) {
  switch (c) {
    case 'W': return '^';
    case 'S': return 'v';
    case 'A': return '<';
    case 'D': return '>';
    default:  return '?';
  }
}



/**
 * Retorna cur_row + 1 si la variable c es 'v', 's' o 'S'.
 * Retorna cur_row - 1 si la variable c es '^', 'w' o 'W'.
 * Retorna cur_row de lo contrario
*/
static unsigned int get_next_row(unsigned int cur_row, char c) {
  if (c == 'v' || c == 's' || c == 'S') return cur_row + 1;
  if (c == '^' || c == 'w' || c == 'W') return cur_row - 1;
  return cur_row;
}



/**
 * Retorna cur_col + 1 si la variable c es '>' or 'd' or 'D'.
 * Retorna cur_col - 1 si la variable c es '<' or 'a' or 'A'.
 * Retorna cur_col de lo contrario
*/
static unsigned int get_next_col(unsigned int cur_col, char c) {
  if (c == '>' || c == 'd' || c == 'D') return cur_col + 1;
  if (c == '<' || c == 'a' || c == 'A') return cur_col - 1;
  return cur_col;
}



/**
 * Tarea 4.2
 *
 * Funcion de ayuda para update_state. Retorna el caracter de la celda
 * en donde la snake se va a mover (en el siguiente paso).
 *
 * Esta funcion no deberia modificar nada de state.
*/
static char next_square(game_state_t* state, unsigned int snum) {
  snake_t s = state->snakes[snum];

  // Obtener dirección actual desde la cabeza
  char head_char = state->board[s.head_row][s.head_col];

  // Calcular coordenadas del próximo paso
  unsigned int next_row = get_next_row(s.head_row, head_char);
  unsigned int next_col = get_next_col(s.head_col, head_char);

  return get_board_at(state, next_row, next_col);
}



/**
 * Tarea 4.3
 *
 * Funcion de ayuda para update_state. Actualiza la cabeza de la snake...
 *
 * ... en el tablero: agregar un caracter donde la snake se va a mover (¿que caracter?)
 *
 * ... en la estructura del snake: actualizar el row y col de la cabeza
 *
 * Nota: esta funcion ignora la comida, paredes, y cuerpos de otras snakes
 * cuando se mueve la cabeza.
*/
static void update_head(game_state_t* state, unsigned int snum) {
  snake_t* s = &state->snakes[snum];

  char head_char = get_board_at(state, s->head_row, s->head_col);

  char body_char = head_to_body(head_char);
  set_board_at(state, s->head_row, s->head_col, body_char);

  unsigned int new_row = get_next_row(s->head_row, head_char);
  unsigned int new_col = get_next_col(s->head_col, head_char);

  set_board_at(state, new_row, new_col, head_char);

  s->head_row = new_row;
  s->head_col = new_col;
}



/**
 * Tarea 4.4
 *
 * Funcion de ayuda para update_state. Actualiza la cola de la snake...
 *
 * ... en el tablero: colocar un caracter blanco (spacio) donde se encuentra
 * la cola actualmente, y cambiar la nueva cola de un caracter de cuerpo (^<v>)
 * a un caracter de cola (wasd)
 *
 * ...en la estructura snake: actualizar el row y col de la cola
*/
static void update_tail(game_state_t* state, unsigned int snum) {
  snake_t* s = &state->snakes[snum];

  char tail_char = get_board_at(state, s->tail_row, s->tail_col);

  set_board_at(state, s->tail_row, s->tail_col, ' ');

  unsigned int new_row = get_next_row(s->tail_row, tail_char);
  unsigned int new_col = get_next_col(s->tail_col, tail_char);

  char next = get_board_at(state, new_row, new_col);
  char new_tail = body_to_tail(next);

  set_board_at(state, new_row, new_col, new_tail);

  s->tail_row = new_row;
  s->tail_col = new_col;
}


/* Tarea 4.5 */
void update_state(game_state_t* state, int (*add_food)(game_state_t*)) {
  for (unsigned int i = 0; i < state->num_snakes; ++i) {
    snake_t* s = &state->snakes[i];
    if (!s->live) continue;

    char next = next_square(state, i);

    if (next == ' ') {
      update_head(state, i);
      update_tail(state, i);
    } else if (next == '*') {
      update_head(state, i);
      add_food(state);  
    } else {

      set_board_at(state, s->head_row, s->head_col, 'x');
      s->live = false;
    }
  }
}


/* Tarea 5 */
game_state_t* load_board(char* filename) {
  FILE* f = fopen(filename, "r");
  if (!f) return NULL;

  unsigned int rows_allocated = 8;
  unsigned int rows_used = 0;
  char** board = malloc(sizeof(char*) * rows_allocated);

  char buffer[256];
  while (fgets(buffer, sizeof(buffer), f)) {
    size_t len = strlen(buffer);
    if (buffer[len - 1] == '\n') buffer[--len] = '\0';

    if (rows_used == rows_allocated) {
      rows_allocated *= 2;
      board = realloc(board, sizeof(char*) * rows_allocated);
    }

    board[rows_used] = malloc(len + 1);
    strcpy(board[rows_used], buffer);
    rows_used++;
  }

  fclose(f);

  game_state_t* state = malloc(sizeof(game_state_t));
  state->num_rows = rows_used;
  state->board = board;
  state->num_snakes = 0;
  state->snakes = NULL;

  return state;
}




/**
 * Tarea 6.1
 *
 * Funcion de ayuda para initialize_snakes.
 * Dada una structura de snake con los datos de cola row y col ya colocados,
 * atravezar el tablero para encontrar el row y col de la cabeza de la snake,
 * y colocar esta informacion en la estructura de la snake correspondiente
 * dada por la variable (snum)
*/
static void find_head(game_state_t* state, unsigned int snum) {
  snake_t* s = &state->snakes[snum];
  unsigned int r = s->tail_row;
  unsigned int c = s->tail_col;
  char curr = get_board_at(state, r, c);

  while (!is_head(curr)) {
    r = get_next_row(r, curr);
    c = get_next_col(c, curr);
    curr = get_board_at(state, r, c);
  }

  s->head_row = r;
  s->head_col = c;
}


/* Tarea 6.2 */
game_state_t* initialize_snakes(game_state_t* state) {
  unsigned int capacidad = 4;
  unsigned int cantidad = 0;
  snake_t* serpientes = malloc(sizeof(snake_t) * capacidad);

  for (unsigned int i = 0; i < state->num_rows; ++i) {
    for (unsigned int j = 0; state->board[i][j] != '\0'; ++j) {
      char ch = state->board[i][j];
      if (is_tail(ch)) {
        if (cantidad == capacidad) {
          capacidad *= 2;
          serpientes = realloc(serpientes, sizeof(snake_t) * capacidad);
        }
        serpientes[cantidad].tail_row = i;
        serpientes[cantidad].tail_col = j;
        serpientes[cantidad].live = true;
        find_head(state, cantidad);
        cantidad++;
      }
    }
  }

  state->snakes = serpientes;
  state->num_snakes = cantidad;

  return state;
}

