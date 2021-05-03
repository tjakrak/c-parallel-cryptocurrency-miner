/**
 * @file mine.c
 *
 * Parallelizes the hash inversion technique used by cryptocurrencies such as
 * bitcoin.
 *
 * Input:    Number of threads, block difficulty, and block contents (string)
 * Output:   Hash inversion solution (nonce) and timing statistics.
 *
 * Compile:  (run make)
 *           When your code is ready for performance testing, you can add the
 *           -O3 flag to enable all compiler optimizations.
 *
 * Run:      ./miner 4 24 'Hello CS 521!!!'
 *
 *   Number of threads: 4
 *     Difficulty Mask: 00000000000000000000000011111111
 *          Block data: [Hello CS 521!!!]
 *
 *   ----------- Starting up miner threads!  -----------
 *
 *   Solution found by thread 3:
 *   Nonce: 10211906
 *   Hash: 0000001209850F7AB3EC055248EE4F1B032D39D0
 *   10221196 hashes in 0.26s (39312292.30 hashes/sec)
 */

#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "sha1.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condc = PTHREAD_COND_INITIALIZER;
pthread_cond_t condp = PTHREAD_COND_INITIALIZER;


unsigned long long total_inversions;
char *bitcoin_block_data;
int num_threads;
uint32_t difficulty_mask = 0;


int buffer = 0;
uint64_t last_index = 0;
int range = 100;
struct tasks {
    uint64_t start;
    uint64_t end;
} *tsk;


uint64_t nonce_result = 0;
char solution[41];


double get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

void print_binary32(uint32_t num) {
    int i;
    for (i = 31; i >= 0; --i) {
        uint32_t position = (unsigned int) 1 << i;
        printf("%c", ((num & position) == position) ? '1' : '0');
    }
    puts("");
}

uint64_t mine(char *data_block, uint32_t difficulty_mask,
        uint64_t nonce_start, uint64_t nonce_end,
        uint8_t digest[SHA1_HASH_SIZE]) {

    for (uint64_t nonce = nonce_start; nonce < nonce_end; nonce++) {
        /* A 64-bit unsigned number can be up to 20 characters  when printed: */
        size_t buf_sz = sizeof(char) * (strlen(data_block) + 20 + 1);
        char *buf = malloc(buf_sz);

        /* Create a new string by concatenating the block and nonce string.
         * For example, if we have 'Hello World!' and '10', the new string
         * is: 'Hello World!10' */
        snprintf(buf, buf_sz, "%s%lu", data_block, nonce);
        //printf("%s\n", buf);

        /* Hash the combined string */
        sha1sum(digest, (uint8_t *) buf, strlen(buf));
        free(buf);
        total_inversions++;

        /* Get the first 32 bits of the hash */
        uint32_t hash_front = 0;
        hash_front |= digest[0] << 24;
        hash_front |= digest[1] << 16;
        hash_front |= digest[2] << 8;
        hash_front |= digest[3];

        /* Check to see if we've found a solution to our block */
        if ((hash_front & difficulty_mask) == hash_front) {
            return nonce;
        }
    }

    return 0;
}

void *consumer_thread(void *ptr) {

    while (last_index < UINT64_MAX / range / num_threads && nonce_result == 0) {
        pthread_mutex_lock(&mutex);
        long thd_num = (unsigned long) ptr;

        while (buffer < 1 || last_index >= UINT64_MAX / range / num_threads) {
            if (last_index >= UINT64_MAX / 100 / num_threads || nonce_result != 0) {
                
                printf("Threads %ld EXIT HERE\n", thd_num);
                printf("Last Index = %lu\n", last_index); 

                pthread_mutex_unlock(&mutex); 
                return 0;
            }
            pthread_cond_wait(&condc, &mutex);
        }
        
        int real_index = last_index % 100;
        printf("Threads: %ld Struct value: %lu, %lu\n", thd_num, tsk[real_index].start, tsk[real_index].end);

        uint64_t start = tsk[real_index].start;
        uint64_t end = tsk[real_index].end;

        last_index++;
        buffer--; // Decremented the buffer we have (total number of struct in the array)

        pthread_cond_signal(&condc);
        pthread_mutex_unlock(&mutex);
        
        uint8_t digest[SHA1_HASH_SIZE];

        /* Mine the block. */
        uint64_t nonce = mine(
            bitcoin_block_data,
            difficulty_mask,
            start, end,
            digest); 

        /* When printed in hex, a SHA-1 checksum will be 40 characters. */
        char solution_hash[41];
        sha1tostring(solution_hash, digest);
        
        pthread_mutex_lock(&mutex);
        //sleep(1);
        if (nonce != 0) {
            if (nonce_result == 0 || nonce < nonce_result) {
                strcpy(solution, &solution_hash[0]);
                nonce_result = nonce;
            }
            pthread_cond_broadcast(&condc);
            printf("Hash: %s\n", solution_hash);
        }
        
        printf("lastindex: %lu\n", last_index);
        printf("counter= %d\n", buffer);
        
        /* Wake up the producer */
        pthread_cond_signal(&condp);
        pthread_mutex_unlock(&mutex);
    }

    return 0;
}


int main(int argc, char *argv[]) {
    
    if (argc != 4) {
        printf("Usage: %s threads difficulty 'block data (string)'\n", argv[0]);
        return EXIT_FAILURE;
    }

    // convert number of threads from string to int
    num_threads = (int)strtol(argv[1], NULL, 10);

    if (num_threads < 1) {
        return 1;
    }

    printf("Number of threads: %d\n", num_threads);

    // TODO we have hard coded the difficulty to 20 bits (0x00000FFF). This is a
    // fairly quick computation -- something like 28 will take much longer.  You
    // should allow the user to specify anywhere between 1 and 32 bits of zeros.
    
    int num_difficulty = (int)strtol(argv[2], NULL, 10);
    
    for (int i = 0; i < 32 - num_difficulty; i++) {
        difficulty_mask = difficulty_mask | 1 << i;
    }
    
    printf("  Difficulty Mask: ");
    print_binary32(difficulty_mask);
    

    /* We use the input string passed in (argv[3]) as our block data. In a
     * complete bitcoin miner implementation, the block data would be composed
     * of bitcoin transactions. */
    bitcoin_block_data = argv[3];
    printf("       Block data: [%s]\n", bitcoin_block_data);

    printf("\n----------- Starting up miner threads!  -----------\n\n");

    double start_time = get_time();

    // generate little task (creating a struct)
    // struct - range (start and end)
    tsk = malloc(100 * sizeof(struct tasks));
    
    pthread_t *consumers = malloc(sizeof(pthread_t) * num_threads);
    int i;
    for (i = 0; i < num_threads; i++) {
        pthread_create(&consumers[i], NULL, consumer_thread, (void *) (unsigned long) i);
    }

    /* Producer */
    uint64_t j = 0;    
    while (j < UINT64_MAX / range && nonce_result == 0) {
        j++;

        pthread_mutex_lock(&mutex);

        while (buffer >= 100 || nonce_result != 0) {
            if (nonce_result != 0) {
                break;
            }
            pthread_cond_wait(&condp, &mutex);
        }       
        
        int idx = (j - 1) % 100;
        
        tsk[idx].start = (j - 1) * range + 1;
        tsk[idx].end = tsk[idx].start + range - 1;
        
        printf("start = %lu, end = %lu\n", tsk[idx].start, tsk[idx].end);

        buffer++;
//        printf("buffer = %d\n", buffer);
       
        pthread_cond_signal(&condc); 
        pthread_mutex_unlock(&mutex);
    }



    int k;
    for (k = 0; k < num_threads; k++) {
        printf("join: %d\n", k);
        pthread_join(consumers[k], NULL);
    }


    printf("final buffer= %d\n", buffer);

    double end_time = get_time();

    if (nonce_result == 0) {
        printf("No solution found!\n");
        return 1;
    }

    printf("Solution found by thread %d:\n", 0);
    printf("Nonce: %lu\n", nonce_result);
//    printf(" Hash: %s\n", solution_hash);

    printf("Solution hash %s\n", solution);

    double total_time = end_time - start_time;
    printf("%llu hashes in %.2fs (%.2f hashes/sec)\n",
            total_inversions, total_time, total_inversions / total_time);

    free(consumers);
    free(tsk);

    return 0;
}
