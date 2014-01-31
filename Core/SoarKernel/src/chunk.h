/*************************************************************************
 * PLEASE SEE THE FILE "license.txt" (INCLUDED WITH THIS SOFTWARE PACKAGE)
 * FOR LICENSE AND COPYRIGHT INFORMATION.
 *************************************************************************/

/* =======================================================================
                                chunk.h
======================================================================= */

#ifndef CHUNK_H
#define CHUNK_H

#include "kernel.h"

typedef struct condition_struct condition;
typedef struct instantiation_struct instantiation;
typedef struct symbol_struct Symbol;
typedef struct action_struct action;

/* RBD Need more comments here */
#define CHUNK_COND_HASH_TABLE_SIZE 1024
#define LOG_2_CHUNK_COND_HASH_TABLE_SIZE 10

typedef struct chunk_cond_struct {
  condition *cond;                /* points to the original condition */

  condition *instantiated_cond;   /* points to cond in chunk instantiation */
  condition *variablized_cond;    /* points to cond in the actual chunk */
  condition *saved_prev_pointer_of_variablized_cond; /* don't ask */

  /* dll of all cond's in a set (i.e., a chunk_cond_set, or the grounds) */
  struct chunk_cond_struct *next, *prev;

  /* dll of cond's in this particular hash bucket for this set */
  struct chunk_cond_struct *next_in_bucket, *prev_in_bucket;

  uint32_t hash_value;             /* equals hash_condition(cond) */
  uint32_t compressed_hash_value;  /* above, compressed to a few bits */
} chunk_cond;


typedef struct chunk_cond_set_struct {
  chunk_cond *all;       /* header for dll of all chunk_cond's in the set */
  chunk_cond *table[CHUNK_COND_HASH_TABLE_SIZE];  /* hash table buckets */
} chunk_cond_set;

typedef struct agent_struct agent;

extern void init_chunker (agent* thisAgent);
extern void chunk_instantiation (agent* thisAgent,
								                 instantiation *inst,
                                 bool variablize,
                                 instantiation **custom_inst_list);
extern chunk_cond *make_chunk_cond_for_negated_condition (agent* thisAgent, condition *cond);
extern bool add_to_chunk_cond_set (agent* thisAgent, chunk_cond_set *set, chunk_cond *new_cc);
//extern bool reverse_unbound_lhs_referents (agent* thisAgent, condition **lhs_top);

#endif


