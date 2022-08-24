#include "markov_chain.h"

/**
* Get random number between 0 and max_number [0, max_number).
* @param max_number maximal number to return (not including)
* @return Random number
*/
int get_random_number (int max_number)
{
  return rand () % max_number;
}

MarkovNode *get_first_random_node (MarkovChain *markov_chain)
{
  Node *iter = NULL;
  do
    {
      iter = markov_chain->database->first;
      int rand = get_random_number (markov_chain->database->size);
      for (int i = 0; i < rand; ++i)
        {
          iter = iter->next;
        }
    }
  while (markov_chain->is_last(iter->data->data));

  return iter->data;
}

/**
 * Count the combined number of cells in the node's counter_list
 * @param state_struct_ptr MarkovNode to count from
 * @return sum of nodes in counter_list
 */
int get_counter_list_sum(MarkovNode *state_struct_ptr) {
  int sum = 0;
  NextNodeCounter *iter1 = state_struct_ptr->counter_list;
  for (int j = 0; j < state_struct_ptr->counter_list_length; ++j)
    {
      sum += iter1->frequency;
      iter1 += 1;
    }
  return sum;
}

MarkovNode *get_next_random_node (MarkovNode *state_struct_ptr)
{
  int sum = get_counter_list_sum(state_struct_ptr);
  int r = get_random_number (sum);

  NextNodeCounter *iter = state_struct_ptr->counter_list;
  while (r >= iter->frequency)
    {
      r -= iter->frequency;
      iter += 1;
    }

  return iter->markov_node->data;
}

void generate_random_sequence (MarkovChain *markov_chain,
                               MarkovNode *first_node,
                               int max_length)
{
  MarkovNode *next = NULL;
  if (!first_node)
    {
      next = get_first_random_node (markov_chain);
    }
  else
    {
      next = first_node;
    }
  markov_chain->print_func(next->data);
  max_length--;
  do
    {
      next = get_next_random_node (next);
      markov_chain->print_func(next->data);
    }
  while (--max_length > 0 && !markov_chain->is_last (next->data));
  printf ("\n");
}

MarkovChain *create_markov_chain ()
{
  MarkovChain *markov_chain = calloc (1, sizeof (MarkovChain));
  if (!markov_chain)
    {
      printf ("%s", ALLOCATION_ERROR_MASSAGE);
      return NULL;
    }
  markov_chain->database = calloc (1, sizeof (LinkedList));
  if (!markov_chain->database)
    {
      printf ("%s", ALLOCATION_ERROR_MASSAGE);
      free (markov_chain);
      return NULL;
    }

  return markov_chain;
}


void free_markov_chain (MarkovChain **ptr_chain)
{
  Node *chain_iter = (*ptr_chain)->database->first;
  Node *temp = NULL;
  for (int i = 0; i < (*ptr_chain)->database->size; ++i)
    {


      free (chain_iter->data->counter_list);
      (*ptr_chain)->free_data(chain_iter->data->data);
      free (chain_iter->data);

      temp = chain_iter;
      chain_iter = chain_iter->next;
      free (temp);
    }

  free ((*ptr_chain)->database);
  free (*ptr_chain);
}


/**
 * Create a new counter list if empty, and add second_node to the
 *  counter list of first_node
 * @param first_node the node with the new list to create
 * @param second_node the node to add to the list
 * @param markov_chain the database containing the markov chain.
 * @return success/failure: true if the process was successful, false if in
 * case of allocation error.
 */
bool create_new_counter_list(MarkovNode *first_node,
                             MarkovNode *second_node,
                             MarkovChain *markov_chain)
{
  NextNodeCounter*  new_list = calloc (1, sizeof (NextNodeCounter));
  if (!new_list)
    {
      printf ("%s", ALLOCATION_ERROR_MASSAGE);
      return false;
    }
  new_list->markov_node = get_node_from_database (markov_chain,
                                             second_node->data);
  new_list->frequency = 1;
  first_node->counter_list = new_list;
  first_node->counter_list_length += 1;

  return true;
}

/**
 * Iterate over the first_node's chain. If the chain contains the
 * second_node's data, then increment it's frequency by 1.
 * @param first_node the node with the list to iterate
 * @param second_node the node with the data we are looking for
 * @return success/failure: true if the process was successful, false if
 * word is not found
 */
bool word_found_in_counter_list(MarkovNode *first_node,
                                MarkovNode *second_node)
{
  int i = 0;
  NextNodeCounter *iter = first_node->counter_list;
  while (i < first_node->counter_list_length)
    {
      if (second_node - iter->markov_node->data == 0)
        {
          iter->frequency += 1;
          return true;
        }
      i++;
      iter = first_node->counter_list + i;
    }
    return false;
}

/**
 * Add a new node to the counter_list of first_node.
 * @param first_node the node with the counter_list to add to
 * @param second_node the node to add to the list
 * @param markov_chain the database containing the markov chain.
 * @return success/failure: true if the process was successful, false if in
 * case of allocation error.
 */
bool add_new_node_to_counter_list(MarkovNode *first_node,
                                  MarkovNode *second_node,
                                  MarkovChain *markov_chain)
{
  int len = first_node->counter_list_length;
  first_node->counter_list = realloc (first_node->counter_list,
                                      (len + 1) * sizeof (NextNodeCounter));
  if (!first_node->counter_list)
    {
      printf ("%s", ALLOCATION_ERROR_MASSAGE);
      return false;
    }
  NextNodeCounter *new_node = first_node->counter_list + len;
  new_node->markov_node = get_node_from_database (markov_chain,
                                              second_node->data);
  new_node->frequency = 1;
  first_node->counter_list_length += 1;

  return true;
}

bool add_node_to_counter_list (MarkovNode *first_node,
                               MarkovNode *second_node,
                               MarkovChain *markov_chain)
{
  if (first_node->counter_list_length == 0)
    {
      return create_new_counter_list(first_node, second_node, markov_chain);
    }
  else
    {
      if (!word_found_in_counter_list(first_node, second_node)) {
          add_new_node_to_counter_list(first_node, second_node, markov_chain);
      }
    }

  return true;
}

Node *get_node_from_database (MarkovChain *markov_chain, void *data_ptr)
{
  if (!data_ptr || !markov_chain->database->first)
    {
      return NULL;
    }
  Node *iter = markov_chain->database->first;
  while (markov_chain->comp_func(iter->data->data, data_ptr) != 0)
    {
      iter = iter->next;
      if (!iter)
        {
          return NULL;
        }
    }

  return iter;
}

/**
 * Create a new markov_node, and returns it.
 * @param data_ptr the string to add to the node's data
 * @return the new markov_node
 * returns NULL in case of memory allocation failure.
 */
MarkovNode *create_markov_node (MarkovChain *markov_chain, void *data_ptr)
{
  MarkovNode *markov_node = calloc (1, sizeof (MarkovNode));
  markov_node->data = markov_chain->copy_func(data_ptr);

  return markov_node;
}

Node *add_to_database (MarkovChain *markov_chain, void *data_ptr)
{
  Node *node = get_node_from_database (markov_chain, data_ptr);
  if (!node)
    {
      MarkovNode *markov_node = create_markov_node (markov_chain, data_ptr);
      if (!markov_node || add (markov_chain->database, markov_node) == 1)
        {
          printf ("%s", ALLOCATION_ERROR_MASSAGE);
          free_markov_chain (&markov_chain);
          return NULL;
        }
      node = get_node_from_database (markov_chain, data_ptr);
    }
  return node;
}
