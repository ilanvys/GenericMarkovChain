#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "markov_chain.h"

#define FILE_ERR_MSG "ERROR: The given file is invalid.\n"
#define USAGE_ERR_MSG "USAGE: Incorrect num of arguments"
#define DELIMITERS " \n\r"
#define DECIMAL_BASE 10
#define MIN_ARGS_NUM 4
#define MAX_ARGS_NUM 5
#define MAX_TWEET_LENGTH 20
#define BUFFER_LENGTH 1000

static int validate_args (int argc, char *argv[]);
static int get_num_from_str (char *str);
static Node *add_first_word_to_chain (MarkovChain *markov_chain, char *line);
static int handle_line (MarkovChain *markov_chain,
                        char *line,
                        int words_left);
static int fill_database (FILE *fp,
                           int words_to_read,
                           MarkovChain *markov_chain);
static void generate_tweets (MarkovChain *markov_chain,
                      int tweets_num,
                      int tweet_size);
static int fill_database_wrapper (FILE *fp,
                            char *words_to_read_arg,
                            MarkovChain *markov_chain);
static MarkovChain *get_markov_chain ();

// functions for generic implementation
static void print_word (void *data);
static int compare_words (void *ptr1, void *ptr2);
static void free_word (void *data);
static void *copy_word (void *ptr);
static bool is_last_word (void *ptr);


int main (int argc, char *argv[])
{
  if (validate_args (argc, argv) != 0)
    {
      return EXIT_FAILURE;
    }

  // Set seed for rand
  srand ((int) get_num_from_str (argv[1]));
  int tweets_num = get_num_from_str (argv[2]);
  FILE *text_corpus = fopen (argv[3], "r");
  MarkovChain *markov_chain = get_markov_chain ();
  if (!markov_chain)
    {
      return EXIT_FAILURE;
    }
  if (fill_database_wrapper (text_corpus, argv[4], markov_chain) != 0) {
    return EXIT_FAILURE;
  }

  generate_tweets (markov_chain, tweets_num, MAX_TWEET_LENGTH);
  free_markov_chain (&markov_chain);

  return EXIT_SUCCESS;
}

/**
 * Validate the arguments that the program received
 * @param argc num of arguments
 * @param argv array of pointers to the arguments
 * @return EXIT_SUCCESS if the arguments are valid, EXIT_FAILURE
 * in case of invalid arguments.
 */
static int validate_args (int argc, char *argv[])
{
  if (argc != MAX_ARGS_NUM && argc != MIN_ARGS_NUM)
    {
      fprintf (stdout, USAGE_ERR_MSG);
      return EXIT_FAILURE;
    }

  FILE *text_corpus = fopen (argv[3], "r");
  if (!text_corpus)
    {
      fprintf (stderr, FILE_ERR_MSG);
      return EXIT_FAILURE;
    }
  fclose (text_corpus);

  return EXIT_SUCCESS;
}

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
 * Receives a line as a string and adds adds the first word in the line
 * to the markov chain (a line will always have at least two words).
 * @param markov_chain the database containing the markov chain.
 * @param line pointer to a string
 * @return markov_node wrapping given data_ptr in given chain's database,
 * returns NULL in case of memory allocation failure.
 */
static Node *add_first_word_to_chain (MarkovChain *markov_chain, char *line)
{
  char *word;
  word = strtok (line, DELIMITERS);
  if (!word)
    {
      return NULL;
    }
  return add_to_database (markov_chain, word);
}

/**
 * Receives a line as a string and adds all the data in the line to the
 * markov chain.
 * @param markov_chain the database containing the markov chain.
 * @param line pointer to a string
 * @param words_left the number of words left to read until reached the
 *                   wanted amount
 * @return number of words left to read after the line was handled
 */
static int handle_line (MarkovChain *markov_chain, char *line, int words_left)
{
  char *curr_word;
  Node *n1 = add_first_word_to_chain (markov_chain, line);
  words_left--;

  curr_word = strtok (NULL, DELIMITERS);
  while (curr_word && words_left != 0)
    {
      Node *n2 = add_to_database (markov_chain, curr_word);
      words_left--;
      if (!n1 || !n2)
        {
          return words_left;
        }

      if (!add_node_to_counter_list (n1->data, n2->data, markov_chain))
        {
          free_markov_chain (&markov_chain);
        }

      n1 = n2;
      curr_word = strtok (NULL, DELIMITERS);
    }

  return words_left;
}

/**
 * Fills Markov Chain from given input
 * @param fp file to read the words from
 * @param words_to_read max number of words to read from file
 * @param markov_chain the database to fill
 * @return EXIT_SUCCESS if the filling the database succeeded,
 * EXIT_FAILURE otherwise.
 */
static int fill_database (FILE *fp,
                           int words_to_read,
                           MarkovChain *markov_chain)
{
  char line[BUFFER_LENGTH];
  if (!fp || words_to_read == 0)
    {
      return EXIT_FAILURE;
    }
  while (fgets (line, BUFFER_LENGTH, fp) && words_to_read != 0)
    {
      words_to_read = handle_line (markov_chain, line, words_to_read);
    }
  fclose (fp);
  return EXIT_SUCCESS;
}

/**
 * Receives a Markov Chain, generates and prints the amount of
 * tweets requested.
 * @param markov_chain a representation of a markov chain
 * @param tweets_num number of tweets to create
 * @param tweet_size the max size for each tweet.
 */
static void generate_tweets (MarkovChain *markov_chain,
                      int tweets_num,
                      int tweet_size)
{
  for (int j = 1; j <= tweets_num; ++j)
    {
      printf ("Tweet %d: ", j);
      generate_random_sequence (markov_chain,
                                get_first_random_node (markov_chain),
                                tweet_size);
    }
}

/**
 * Wrapper function to 'fill_database'. Helps send the correct parameters
 * based on whether you like it or not
 * @param fp file to read the words from
 * @param words_to_read_arg pointer to str of max number of words to
 *                          read from file
 * @param markov_chain the database to fill
 * @return EXIT_SUCCESS if the filling the database succeeded,
 * EXIT_FAILURE otherwise.
 */
static int fill_database_wrapper (FILE *fp,
                            char *words_to_read_arg,
                            MarkovChain *markov_chain)
{
  if (words_to_read_arg)
    {
      int words_to_read = get_num_from_str (words_to_read_arg);
      return fill_database (fp, words_to_read, markov_chain);
    }
  else
    {
      return fill_database (fp, -1, markov_chain);
    }
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

  markov_chain->print_func = print_word;
  markov_chain->comp_func = compare_words;
  markov_chain->free_data = free_word;
  markov_chain->copy_func = copy_word;
  markov_chain->is_last = is_last_word;
  return markov_chain;
}

// print
static void print_word (void *data)
{
  printf ("%s", (char *) data);
  if (!is_last_word (data))
    {
      printf(" ");
    }
}

// compare
static int compare_words (void *ptr1, void *ptr2)
{
  return strcmp ((char *) ptr1, (char *) ptr2);
}

// free data
static void free_word (void *data)
{
  free ((char *) data);
}

// copy
static void *copy_word (void *ptr)
{
  void *dest = calloc ((strlen ((char *) ptr) + 1), sizeof (char));
  if (!dest)
    {
      return NULL;
    }
  memcpy (dest, ptr, strlen ((char *) ptr) + 1);
  return dest;
}

// is last
static bool is_last_word (void *ptr)
{
  char *str = (char *) ptr;
  unsigned long len = strlen (str);
  if ('.' == str[len - 1])
    {
      return true;
    }
  return false;
}
