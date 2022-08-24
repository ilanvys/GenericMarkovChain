tweets: linked_list.c markov_chain.c tweets_generator.c
	gcc linked_list.c markov_chain.c tweets_generator.c  -o tweets_generator

snake: linked_list.c markov_chain.c snakes_and_ladders.c
	gcc linked_list.c markov_chain.c snakes_and_ladders.c -o snakes_and_ladders