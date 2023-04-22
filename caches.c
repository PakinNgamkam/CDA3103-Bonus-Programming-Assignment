#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define CACHE_SIZE 32
#define BLOCK_SIZE 4
#define NUM_BLOCKS (CACHE_SIZE / BLOCK_SIZE)
#define NUM_WAYS_2 2
#define NUM_WAYS_4 4

typedef enum { DIRECT_MAPPED, TWO_WAY, FOUR_WAY, FULLY_ASSOC } CacheType;
typedef enum { LRU, RANDOM } ReplacementPolicy;

typedef struct {
    uint32_t tag;
    bool valid;
    int counter;
} CacheLine;

typedef struct {
    CacheType type;
    ReplacementPolicy policy;
    int num_sets;
    int num_ways;
    CacheLine *cache_lines;
} Cache;

uint32_t get_tag(uint32_t address);
uint32_t get_index(uint32_t address, int num_sets);
bool access_cache(Cache *cache, uint32_t address);
void update_counters(Cache *cache, int set_idx, int way_idx);
int find_replacement(Cache *cache, int set_idx);

int main() {
    FILE *file = fopen("traces.txt", "r");
    if (file == NULL) {
        printf("Error: Cannot open file.\n");
        return 1;
    }

    Cache configs[8];
    for (int i = 0; i < 8; i++) {
        configs[i].type = (CacheType)(i / 2);
        configs[i].policy = (ReplacementPolicy)(i % 2);
        configs[i].num_sets = (configs[i].type == DIRECT_MAPPED) ? NUM_BLOCKS :
                              (configs[i].type == TWO_WAY) ? NUM_BLOCKS / NUM_WAYS_2 :
                              (configs[i].type == FOUR_WAY) ? NUM_BLOCKS / NUM_WAYS_4 : 1;
        configs[i].num_ways = (configs[i].type == DIRECT_MAPPED) ? 1 :
                              (configs[i].type == TWO_WAY) ? NUM_WAYS_2 :
                              (configs[i].type == FOUR_WAY) ? NUM_WAYS_4 : NUM_BLOCKS;
        configs[i].cache_lines = (CacheLine *)calloc(configs[i].num_sets * configs[i].num_ways, sizeof(CacheLine));
    }

    srand(time(NULL));

    uint32_t address;
    int num_accesses = 0;
    int hits[8] = {0};
    int scan_result;

    while ((scan_result = fscanf(file, "%x", &address)) != EOF) {
        // printf("Address: 0x%x\n", address);
        for (int i = 0; i < 8; i++) {
            if (access_cache(&configs[i], address)) {
                hits[i]++;
            }
        }
        num_accesses++;
    }
    // printf("Scan result: %d\n", scan_result);

    fclose(file);

    for (int i = 0; i < 8; i++) {
        free(configs[i].cache_lines);
        printf("Cache Config %d: Type = %s, Policy = %s\n",
               i + 1,
               (configs[i].type == DIRECT_MAPPED) ? "Direct Mapped" :
               (configs[i].type == TWO_WAY) ? "2-Way" :
               (configs[i].type == FOUR_WAY) ? "4-Way" : "Fully Associative",
               (configs[i].policy == LRU) ? "LRU" : "Random");
        printf("Number of Hits: %d\n", hits[i]);
        printf("Number of Total Accesses: %d\n", num_accesses);
        printf("Hit Rate: %.2f%%\n\n", (float)hits[i] / num_accesses * 100);
    }

    return 0;
}

uint32_t get_tag(uint32_t address) {
    return address / NUM_BLOCKS;
}

uint32_t get_index(uint32_t address, int num_sets) {
    return (address / BLOCK_SIZE) % num_sets;
}

bool access_cache(Cache *cache, uint32_t address) {
    uint32_t tag = get_tag(address);
    uint32_t index = get_index(address, cache->num_sets);

    for (int way = 0; way < cache->num_ways; way++) {
        int idx = index * cache->num_ways + way;
        if (cache->cache_lines[idx].valid && cache->cache_lines[idx].tag == tag) {
            update_counters(cache, index, way);
            return true;
        }
    }

    int replacement = find_replacement(cache, index);
    cache->cache_lines[replacement].tag = tag;
    cache->cache_lines[replacement].valid = true;
    update_counters(cache, index, replacement % cache->num_ways);
    return false;
}

void update_counters(Cache *cache, int set_idx, int way_idx) {
    if (cache->policy == LRU) {
        for (int way = 0; way < cache->num_ways; way++) {
            int idx = set_idx * cache->num_ways + way;
            cache->cache_lines[idx].counter++;
        }
        cache->cache_lines[set_idx * cache->num_ways + way_idx].counter = 0;
    }
}

int find_replacement(Cache *cache, int set_idx) {
    int replacement = -1;
    if (cache->policy == LRU) {
        int max_counter = -1;
        for (int way = 0; way < cache->num_ways; way++) {
            int idx = set_idx * cache->num_ways + way;
            if (!cache->cache_lines[idx].valid) {
                return idx;
            }
            if (cache->cache_lines[idx].counter > max_counter) {
                max_counter = cache->cache_lines[idx].counter;
                replacement = idx;
            }
        }
    } else { // Random replacement policy
        replacement = set_idx * cache->num_ways + (rand() % cache->num_ways);
    }
    return replacement;
}
