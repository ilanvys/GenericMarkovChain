#include <string.h> // For strlen(), strcmp(), strcpy()
#include "markov_chain.h"

#define MAX(X, Y) (((X) < (Y)) ? (Y) : (X))

#define EMPTY -1
#define BOARD_SIZE 100
#define MAX_GENERATION_LENGTH 60

#define DICE_MAX 6
#define NUM_OF_TRANSITIONS 20

#define DECIMAL_BASE 10
#define ARGS_NUM 3
#define USAGE_ERR_MSG "USAGE: Incorrect num of arguments"

/**
 * represents the transitions by ladders and snakes in the game
 * each tuple (x,y) represents a ladder from x to if x<y or a snake otherwise
 */
const int transitions[][2] = {{13, 4},
                              {85, 17},
                              {95, 67},
                              {97, 58},
                              {66, 89},
                              {87, 31},
                              {57, 83},
                              {91, 25},
                              {28, 50},
                              {35, 11},
                              {8,  30},
                              {41, 62},
                              {81, 43},
                              {69, 32},
                              {20, 39},
                              {33, 70},
                              {79, 99},
                              {23, 76},
                              {15, 47},
                              {61, 14}};

/**
 * struct represents a Cell in the game board
 */
typedef struct Cell {
    // Cell number 1-100
    int number;
    // ladder_to represents the jump of the ladder in case there is
    // one from this square
    int ladder_to;
    // snake_to represents the jump of the snake in case
    // there is one from this square
    int snake_to;
    //both ladder_to and snake_to should be -1 if the Cell doesn't have them
} Cell;

// functions for generic implementation
static void print_cell (void *data);
static int compare_cells (void *ptr1, void *ptr2);
static void free_cell (void *data);
static void *copy_cell (void *ptr);
static bool is_last_cell (void *ptr);

// supplied functions
static int handle_error (char *error_msg, MarkovChain **database);
static int create_board (Cell *cells[BOARD_SIZE]);
static int fill_database (MarkovChain *markov_chain);

// helpers
static int get_num_from_str (char *str);
static MarkovChain *get_markov_chain ();
static void generate_routes (MarkovChain *markov_chain,
                             int routes_num,
                             int routes_size);

/**
 * @param argc num of arguments
 * @param argv 1) Seed
 *             2) Number of sentences to generate
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
int main (int argc, char *argv[])
{
  if (argc != ARGS_NUM)
    {
      fprintf (stdout, USAGE_ERR_MSG);
      return EXIT_FAILURE;
    }

  // Set seed for rand
  srand ((int) get_num_from_str (argv[1]));
  int routes_num = get_num_from_str (argv[2]);
  MarkovChain *markov_chain = get_markov_chain ();
  if (!markov_chain)
    {
      return EXIT_FAILURE;
    }

  fill_database (markov_chain);
  generate_routes (markov_chain, routes_num, MAX_GENERATION_LENGTH);
  free_markov_chain (&markov_chain);

  return EXIT_SUCCESS;
}

// functions for generic implementation
// print
static void print_cell (void *data)
{
  Cell *cell = (Cell *) data;
  printf ("[%d]", cell->number);
  if (cell->snake_to != EMPTY)
    {
      printf ("-snake to %d", cell->snake_to);
    }
  if (cell->ladder_to != EMPTY)
    {
      printf ("-ladder to %d", cell->ladder_to);
    }
  if (!is_last_cell(data))
    {
      printf(" -> ");
    }
}

// compare
static int compare_cells (void *ptr1, void *ptr2)
{
  Cell *cell1 = (Cell *) ptr1;
  Cell *cell2 = (Cell *) ptr2;
  return cell1->number - cell2->number;
}

// free data
static void free_cell (void *data)
{
  free ((Cell *) data);
}

// copy
static void *copy_cell (void *ptr)
{
  Cell *src = (Cell *) ptr;
  Cell *dest = calloc (1, sizeof (Cell));
  if (!dest)
    {
      return NULL;
    }
  dest->number = src->number;
  dest->ladder_to = src->ladder_to;
  dest->snake_to = src->snake_to;

  return dest;
}

// is last
static bool is_last_cell (void *ptr)
{
  if (((Cell *) ptr)->number == BOARD_SIZE)
    {
      return true;
    }
  return false;
}


// supplied functions
/** Error handler **/
static int handle_error (char *error_msg, MarkovChain **database)
{
  printf ("%s", error_msg);
  if (database != NULL)
    {
      free_markov_chain (database);
    }
  return EXIT_FAILURE;
}

static int create_board (Cell *cells[BOARD_SIZE])
{
  for (int i = 0; i < BOARD_SIZE; i++)
    {
      cells[i] = malloc (sizeof (Cell));
      if (cells[i] == NULL)
        {
          for (int j = 0; j < i; j++)
            {
              free (cells[j]);
            }
          handle_error (ALLOCATION_ERROR_MASSAGE, NULL);
          return EXIT_FAILURE;
        }
      *(cells[i]) = (Cell) {i + 1, EMPTY, EMPTY};
    }

  for (int i = 0; i < NUM_OF_TRANSITIONS; i++)
    {
      int from = transitions[i][0];
      int to = transitions[i][1];
      if (from < to)
        {
          cells[from - 1]->ladder_to = to;
        }
      else
        {
          cells[from - 1]->snake_to = to;
        }
    }
  return EXIT_SUCCESS;
}

/**
 * fills database
 * @param markov_chain
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
static int fill_database (MarkovChain *markov_chain)
{
  Cell *cells[BOARD_SIZE];
  if (create_board (cells) == EXIT_FAILURE)
    {
      return EXIT_FAILURE;
    }
  MarkovNode *from_node = NULL, *to_node = NULL;
  size_t index_to;
  for (size_t i = 0; i < BOARD_SIZE; i++)
    {
      add_to_database (markov_chain, cells[i]);
    }

  for (size_t i = 0; i < BOARD_SIZE; i++)
    {
      from_node = get_node_from_database (markov_chain, cells[i])->data;

      if (cells[i]->snake_to != EMPTY || cells[i]->ladder_to != EMPTY)
        {
          index_to = MAX(cells[i]->snake_to, cells[i]->ladder_to) - 1;
          to_node = get_node_from_database (markov_chain, cells[index_to])
              ->data;
          add_node_to_counter_list (from_node, to_node, markov_chain);
        }
      else
        {
          for (int j = 1; j <= DICE_MAX; j++)
            {
              index_to = ((Cell *) (from_node->data))->number + j - 1;
              if (index_to >= BOARD_SIZE)
                {
                  break;
                }
              to_node = get_node_from_database (markov_chain, cells[index_to])
                  ->data;
              add_node_to_counter_list (from_node, to_node, markov_chain);
            }
        }
    }
  // free temp arr
  for (size_t i = 0; i < BOARD_SIZE; i++)
    {
      free (cells[i]);
    }
  return EXIT_SUCCESS;
}


// helpers
/**
 * Takes a str and turns it into an integer using strtol.
 * @param str the string
 * @return the integer that was represented as a string.
 */
static int get_num_from_str (char *str)
{
  return (int) strtol (str, NULL, DECIMAL_BASE);
}

/**
 * Creates an instance of Markov Chain, and applies the correct
 * functions for the generic implementation
 * @return a pointer to a MarkovChain, NULL if memory allocation failed.
 */
static MarkovChain *get_markov_chain ()
{
  MarkovChain *markov_chain = create_markov_chain();
  if (!markov_chain)
    {
      printf ("%s", ALLOCATION_ERROR_MASSAGE);
      return NULL;
    }

  markov_chain->print_func = print_cell;
  markov_chain->comp_func = compare_cells;
  markov_chain->free_data = free_cell;
  markov_chain->copy_func = copy_cell;
  markov_chain->is_last = is_last_cell;
  return markov_chain;
}

/**
 * Receives a Markov Chain, generates and prints the amount of
 * routes requested.
 * @param markov_chain a representation of a markov chain
 * @param routes_num number of routes to create
 * @param routes_size the max size for each route.
 */

static void generate_routes (MarkovChain *markov_chain,
                             int routes_num,
                             int routes_size)
{
  for (int j = 1; j <= routes_num; ++j)
    {
      printf ("Random Walk %d: ", j);
      generate_random_sequence (markov_chain,
                                markov_chain->database->first->data,
                                routes_size);
    }
}
