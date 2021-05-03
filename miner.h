/**
 * @file
 * produce structs of block+nonces and consume those through mining it
 */

#ifndef _MINER_H_
#define _MINER_H_

#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "sha1.h"

/**
 * Retrieves the current wall clock time, in seconds.
 *
 * @return current wall clock time
 */
double get_time();

/**
 * Prints a 32-bit unsigned integer as binary digits.
 *
 * @param num The 32-bit unsigned integer to print
 */
void print_binary32(uint32_t num);

/**
 * Finding the number of nonces that produce the binary digits that we are looking for
 *
 * @param data_block input String from the user
 * @param difficulty_mask the number of zeroes we need to find
 * @param nonce_start the start of the nonces to be added to the string
 * @param nonce_end the end of the nonces to be added to the string
 * @param digest
 */
uint64_t mine(char *data_block, uint32_t difficulty_mask,
        uint64_t nonce_start, uint64_t nonce_end,
        uint8_t digest[SHA1_HASH_SIZE]);


#endif
