/**
 * @file markov_corpus.h
 * @brief Markov chain speech and thought synthesizer for emotional dialogue.
 */

#ifndef LORE_MARKOV_CORPUS_H
#define LORE_MARKOV_CORPUS_H

#include "src/types/lore_types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void generateMarkovText(Expression expr, char* out_buf, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* LORE_MARKOV_CORPUS_H */
