#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zlib.h>

#define LO_ITEM 128
#define HI_ITEM 256
#define LO_MAX 124
#define HI_MAX 252
#define NUM_ITER 25

typedef struct c_stats {
  unsigned char med;
  unsigned char max;
  unsigned char min;
  unsigned char avg;
} c_stats;

typedef struct s_stats {
  unsigned short med;
  unsigned short max;
  unsigned short min;
  unsigned short avg;
} s_stats;

int compare_c(const void *a, const void *b) {
  // Convert void pointers to int pointers and dereference them
  unsigned char int_a = *((unsigned char *)a);
  unsigned char int_b = *((unsigned char *)b);

  // Compare the integers
  if (int_a < int_b)
    return -1;
  if (int_a > int_b)
    return 1;
  return 0;
}

int compare_s(const void *a, const void *b) {
  // Convert void pointers to int pointers and dereference them
  unsigned short int_a = *((unsigned short *)a);
  unsigned short int_b = *((unsigned short *)b);

  // Compare the integers
  if (int_a < int_b)
    return -1;
  if (int_a > int_b)
    return 1;
  return 0;
}

c_stats *get_stats_c(unsigned char arr[], int items) {

  unsigned char *dest = (unsigned char *)malloc(items);
  c_stats *r = (c_stats *)malloc(sizeof(c_stats));

  memcpy(dest, arr, items);

  qsort(dest, items, sizeof(unsigned char), compare_c);

  unsigned long avg = 0;
  for (int i = 0; i < items; i++) {
    avg += (unsigned long)dest[i];
  }
  avg /= items;

  r->med = dest[(items / 2)];
  r->max = dest[(items - 1)];
  r->min = dest[0];
  r->avg = avg;

  free(dest);

  return r;
}

s_stats *get_stats_s(unsigned short arr[], int items) {

  unsigned short *dest = (unsigned short *)malloc(items * 2);
  s_stats *r = (s_stats *)malloc(sizeof(s_stats));

  memcpy(dest, arr, items * 2);

  qsort(dest, items, sizeof(unsigned short), compare_s);

  unsigned long long avg = 0;
  for (int i = 0; i < items; i++) {
    avg += (unsigned long long)dest[i];
  }
  avg /= items;

  r->med = (unsigned short)dest[(items / 2)];
  r->max = (unsigned short)dest[(items - 1)];
  r->min = (unsigned short)dest[0];
  r->avg = (unsigned short)avg;

  free(dest);

  return r;
}

// Uses a 9-bit number for storing a counter, and when being compressed it is
// split (1 | 8)
void split_9bit_sim(int items, int max_size) {

  unsigned long total_increments = 0;
  s_stats *s_stats_arr[NUM_ITER];

  for (int iter = 0; iter < NUM_ITER; iter++) {

    int hb_size = items / 8;

    unsigned short ints[items]; // 512B uncompressed array

    unsigned char lo_bits[items];   // 256B array to be concatenated
    unsigned char hi_bits[hb_size]; // 64B array to be concatenated

    unsigned char combined[items + hb_size]; // 320B array to be compressed

    srand(time(NULL));

    // Initialize short array
    for (int i = 0; i < items; i++) {
      ints[i] = 0;
      lo_bits[i] = 0;
    }

    for (int i = 0; i < hb_size; i++) {
      hi_bits[i] = 0;
    }

    Bytef *compr;         // compression buffer
    uLongf compr_len = 0; // length of compression buffer
    int incr_cntr = 0;    // an address access incrementation counter

    while (compr_len < max_size) {
      // while (incr_cntr < 10) {
      int index = rand() % items;

      if (ints[index] + 1 == 512) {
        // printf("\nOverflow at index %i\n", index); // msg about overflow
        goto end_9bit_split;
      }

      ints[index]++;

      // printf("\nsplit(%i, %iB) : (%i accesses)  : Printing short arr\n",
      // items,
      //        max_size, incr_cntr);
      // for (int i = 0; i < items; i++) {
      //   if (i % 16 == 0) {
      //     printf("\n");
      //   }
      //   printf("%i ", ints[i]);
      // } // end of print for loop
      // printf("\n");

      // int h_ind = 0;
      for (int i = 0; i < items; i += 8) {
        int fst = i;
        int snd = i + 1;
        int trd = i + 2;
        int frh = i + 3;
        int fth = i + 4;
        int sxh = i + 5;
        int svh = i + 6;
        int eth = i + 7;

        // these are 9-bit numbers: hope to split into hi 1-bit and lo 8-bit
        unsigned char a = (unsigned char)((ints[fst] & 0x0100) >> 1);
        unsigned char b = (unsigned char)((ints[snd] & 0x0100) >> 2);
        unsigned char c = (unsigned char)((ints[trd] & 0x0100) >> 3);
        unsigned char d = (unsigned char)((ints[frh] & 0x0100) >> 4);
        unsigned char e = (unsigned char)((ints[fth] & 0x0100) >> 5);
        unsigned char f = (unsigned char)((ints[sxh] & 0x0100) >> 6);
        unsigned char g = (unsigned char)((ints[svh] & 0x0100) >> 7);
        unsigned char h = (unsigned char)((ints[eth] & 0x0100) >> 8);

        unsigned char hi = (unsigned char)(a | b | c | d | e | f | g | h);
        // printf("%02X\n", hi);

        hi_bits[i / 8] = hi;

      } // end of split for loop

      for (int i = 0; i < items; i++) {
        lo_bits[i] = (unsigned char)ints[i] & 0x00ff;
      }

      // Uncomment to see arrays

      // printf("\nsplit(%i, %iB) : (%i accesses)  : Printing high char arr\n",
      //        items, max_size, incr_cntr);
      // for (int i = 0; i < hb_size; i++) {
      //   if (i % 16 == 0) {
      //     printf("\n");
      //   }
      //   printf("%02X ", hi_bits[i]);
      // } // end of print for loop
      // printf("\n");

      // printf("\nsplit(%i, %iB) : (%i accesses)  : Printing lo char arr\n",
      //        items, max_size, incr_cntr);
      // for (int i = 0; i < items; i++) {
      //   if (i % 16 == 0) {
      //     printf("\n");
      //   }
      //   printf("%i ", lo_bits[i]);
      // } // end of print for loop
      // printf("\n");

      // copy over high bits
      for (int i = 0; i < hb_size; i++) {
        combined[i] = hi_bits[i];
      } // end of copy hi_bits for loop

      // copy over low bits
      for (int i = 0; i < items; i++) {
        combined[i + hb_size] = lo_bits[i];
      } // end of copy lo_bits for loop

      // perform compression
      // 1B * (256 + 64) + 1 = 321B
      uLong input_len = sizeof(unsigned char) * (items + hb_size) +
                        1; // Include the null terminator
      Bytef *compr = (Bytef *)malloc(
          input_len * 2); // Allocate memory for compression buffer
      uLongf compr_len = input_len * 2; // Size of the compression buffer

      int ret = compress(compr, &compr_len, (const Bytef *)combined, input_len);

      if (ret == Z_OK) {

        incr_cntr++;
        // printf("\nsplit_9bit(%i, %iB) : (%i accesses) : Current Compression "
        //        "Size = %lu",
        //        items, max_size, incr_cntr, compr_len);

        if (compr_len >= max_size) {
        end_9bit_split:
          total_increments += incr_cntr;
          s_stats *stats = get_stats_s(ints, items);
          s_stats_arr[iter] = stats;
          free(compr);
          break;
        } // end of max_size comparison
      } else {
        ints[index]--;
        fprintf(stderr,
                "\nsplit_9bit(%i, %iB): (%i accesses) : Compression failed\n",
                items, max_size, incr_cntr);
      } // end of compression success
    }   // end of while loop
  }     // end of iteration loop

  printf("\nsplit_9bit(%i, %iB) : Average Increments per Iteration = %lu\n",
         items, max_size, total_increments / NUM_ITER);

  unsigned long long avg_med = 0;
  unsigned long long avg_max = 0;
  unsigned long long avg_min = 0;
  unsigned long long avg_avg = 0;

  for (int i = 0; i < NUM_ITER; i++) {
    avg_med += (unsigned long long)s_stats_arr[i]->med;
    avg_max += (unsigned long long)s_stats_arr[i]->max;
    avg_min += (unsigned long long)s_stats_arr[i]->min;
    avg_avg += (unsigned long long)s_stats_arr[i]->avg;
  }

  avg_med /= NUM_ITER;
  avg_max /= NUM_ITER;
  avg_min /= NUM_ITER;
  avg_avg /= NUM_ITER;

  printf("\nAVG MED := %llu\nAVG_MAX := %llu\nAVG_MIN := "
         "%llu\nAVG_AVG := %llu\n\n",
         avg_med, avg_max, avg_min, avg_avg);

} // end of function

// Uses a 10-bit number for storing a counter, and when being compressed it is
// split (2 | 8)
void split_10bit_sim(int items, int max_size) {

  unsigned long total_increments = 0;
  s_stats *s_stats_arr[NUM_ITER];

  for (int iter = 0; iter < NUM_ITER; iter++) {
    int hb_size = items / 4;

    unsigned short ints[items]; // 512B uncompressed array

    unsigned char lo_bits[items];   // 256B array to be concatenated
    unsigned char hi_bits[hb_size]; // 64B array to be concatenated

    unsigned char combined[items + hb_size]; // 320B array to be compressed

    srand(time(NULL));

    // Initialize short array
    for (int i = 0; i < items; i++) {
      ints[i] = 0;
      lo_bits[i] = 0;
    }

    for (int i = 0; i < hb_size; i++) {
      hi_bits[i] = 0;
    }

    Bytef *compr;         // compression buffer
    uLongf compr_len = 0; // length of compression buffer
    int incr_cntr = 0;    // an address access incrementation counter

    while (compr_len < max_size) {
      // while (incr_cntr < 10) {
      int index = rand() % items;

      if (ints[index] + 1 == 1024) {
        // printf("\nOverflow at index %i\n", index); // msg about overflow
        goto end_10bit_split;
      }

      ints[index]++;

      // printf("\nsplit(%i, %iB) : (%i accesses)  : Printing short arr\n",
      // items,
      //        max_size, incr_cntr);
      // for (int i = 0; i < items; i++) {
      //   if (i % 16 == 0) {
      //     printf("\n");
      //   }
      //   printf("%i ", ints[i]);
      // } // end of print for loop
      // printf("\n");

      // int h_ind = 0;
      for (int i = 0; i < items; i += 4) {
        int fst = i;
        int snd = i + 1;
        int trd = i + 2;
        int frh = i + 3;

        unsigned char a = (unsigned char)((ints[fst] & 0x0300) >> 8);
        unsigned char b = (unsigned char)((ints[snd] & 0x0300) >> 8);
        unsigned char c = (unsigned char)((ints[trd] & 0x0300) >> 8);
        unsigned char d = (unsigned char)((ints[frh] & 0x0300) >> 8);

        unsigned char hi = (unsigned char)((a << 6) | (b << 4) | (c << 2) | d);
        // printf("%02X\n", hi);

        hi_bits[i / 4] = hi;

      } // end of split for loop

      for (int i = 0; i < items; i++) {
        lo_bits[i] = (unsigned char)ints[i] & 0x00ff;
      }

      // printf("\nsplit(%i, %iB) : (%i accesses)  : Printing high char arr\n",
      //        items, max_size, incr_cntr);
      // for (int i = 0; i < items / 4; i++) {
      //   if (i % 16 == 0) {
      //     printf("\n");
      //   }
      //   printf("%02X ", hi_bits[i]);
      // } // end of print for loop
      // printf("\n");

      // printf("\nsplit(%i, %iB) : (%i accesses)  : Printing lo char arr\n",
      // items,
      //        max_size, incr_cntr);
      // for (int i = 0; i < items; i++) {
      //   if (i % 16 == 0) {
      //     printf("\n");
      //   }
      //   printf("%i ", lo_bits[i]);
      // } // end of print for loop
      // printf("\n");

      // copy over high bits
      for (int i = 0; i < items / 4; i++) {
        combined[i] = hi_bits[i];
      } // end of copy hi_bits for loop

      // copy over low bits
      for (int i = 0; i < items; i++) {
        combined[i + hb_size] = lo_bits[i];
      } // end of copy lo_bits for loop

      // perform compression
      // 1B * (256 + 64) + 1 = 321B
      uLong input_len = sizeof(unsigned char) * (items + hb_size) +
                        1; // Include the null terminator
      Bytef *compr = (Bytef *)malloc(
          input_len * 2); // Allocate memory for compression buffer
      uLongf compr_len = input_len * 2; // Size of the compression buffer

      int ret = compress(compr, &compr_len, (const Bytef *)combined, input_len);

      if (ret == Z_OK) {

        incr_cntr++;
        // printf("\nsplit_10bit(%i, %iB) : (%i accesses) : Current Compression
        // "
        //        "Size = %lu",
        //        items, max_size, incr_cntr, compr_len);

        if (compr_len >= max_size) {
        end_10bit_split:
          total_increments += incr_cntr;
          s_stats *stats = get_stats_s(ints, items);
          s_stats_arr[iter] = stats;
          free(compr);
          break;
        } // end of max_size comparison
      } else {
        ints[index]--;
        fprintf(stderr,
                "\nsplit_10bit(%i, %iB): (%i accesses) : Compression failed\n",
                items, max_size, incr_cntr);
      } // end of compression success
    }   // end of while loop
  }     // end of iteration loop

  printf("\nsplit_10bit(%i, %iB) : Average Increments per Iteration = %lu\n",
         items, max_size, total_increments / NUM_ITER);

  unsigned long long avg_med = 0;
  unsigned long long avg_max = 0;
  unsigned long long avg_min = 0;
  unsigned long long avg_avg = 0;

  for (int i = 0; i < NUM_ITER; i++) {
    avg_med += (unsigned long long)s_stats_arr[i]->med;
    avg_max += (unsigned long long)s_stats_arr[i]->max;
    avg_min += (unsigned long long)s_stats_arr[i]->min;
    avg_avg += (unsigned long long)s_stats_arr[i]->avg;
  }

  avg_med /= NUM_ITER;
  avg_max /= NUM_ITER;
  avg_min /= NUM_ITER;
  avg_avg /= NUM_ITER;

  printf("\nAVG MED := %llu\nAVG_MAX := %llu\nAVG_MIN := "
         "%llu\nAVG_AVG := %llu\n\n",
         avg_med, avg_max, avg_min, avg_avg);

} // end of function

// Uses a 16-bit number for storing a counter, and when being compressed it is
// split (8 | 8)
void split_16bit_sim(int items, int max_size) {

  unsigned long total_increments = 0;
  s_stats *s_stats_arr[NUM_ITER];

  for (int iter = 0; iter < NUM_ITER; iter++) {
    // Seed the random number generator
    srand(time(NULL));

    // Stack-dynamic array to store the address access counters
    unsigned short shorts[items];

    // Iterate through the array and set each counter to 0
    for (int i = 0; i < items; i++) {
      shorts[i] = 0;
    }

    Bytef *compr;         // compression buffer
    uLongf compr_len = 0; // length of compression buffer
    int incr_cntr = 0;    // an address access incrementation counter

    while (compr_len < max_size) {

      // split
      unsigned char hi_char[items];
      unsigned char lo_char[items];
      unsigned char chars[items * 2];
      // perform increment
      int index = rand() % items;

      if ((unsigned short)(shorts[index] + 1) == 0) {
        // printf("\nsplit(%i, %iB)[%i] : (%i accesses) : the data type has "
        //        "overflowed at index = (%i) (1)!\n",
        //        items, max_size, iter, incr_cntr, index);
        goto end_16bit_split;
      }

      shorts[index] += 1;

      for (int i = 0; i < items; i++) {
        hi_char[i] = (unsigned char)((shorts[i] & 0xff00) >> 8);
        lo_char[i] = (unsigned char)shorts[i] & 0x00ff;
        chars[i] = hi_char[i];
        chars[i + items] = lo_char[i];
      }

      // perform compression
      uLong input_len = sizeof(unsigned char) * (items * 2) +
                        1; // Include the null terminator
      Bytef *compr = (Bytef *)malloc(
          input_len * 2); // Allocate memory for compression buffer
      uLongf compr_len = input_len * 2; // Size of the compression buffer

      int ret = compress(compr, &compr_len, (const Bytef *)chars, input_len);

      if (ret == Z_OK) {

        incr_cntr++;

        if (compr_len >= max_size) {
        end_16bit_split:

          // printf("\nsplit_16bit(%i, %iB)[%i] : (%i accesses) : Current
          // Compression "
          //        "Size = %lu",
          //        items, max_size, iter, incr_cntr, compr_len);

          free(compr);

          total_increments += incr_cntr;
          s_stats *stats = get_stats_s(shorts, items);
          s_stats_arr[iter] = stats;
          break;
        }

      } else {
        shorts[index]--;
        fprintf(
            stderr,
            "\nsplit_16bit(%i, %iB)[%i] : (%i accesses) : Compression failed\n",
            items, max_size, iter, incr_cntr);
      }
    }
  }

  printf("\nsplit_16bit(%i, %iB) : Average Increments per Iteration = %lu\n",
         items, max_size, total_increments / NUM_ITER);

  unsigned long long avg_med = 0;
  unsigned long long avg_max = 0;
  unsigned long long avg_min = 0;
  unsigned long long avg_avg = 0;

  for (int i = 0; i < NUM_ITER; i++) {
    avg_med += (unsigned long long)s_stats_arr[i]->med;
    avg_max += (unsigned long long)s_stats_arr[i]->max;
    avg_min += (unsigned long long)s_stats_arr[i]->min;
    avg_avg += (unsigned long long)s_stats_arr[i]->avg;
  }

  avg_med /= NUM_ITER;
  avg_max /= NUM_ITER;
  avg_min /= NUM_ITER;
  avg_avg /= NUM_ITER;

  printf("\nAVG MED := %llu\nAVG_MAX := %llu\nAVG_MIN := "
         "%llu\nAVG_AVG := %llu\n\n",
         avg_med, avg_max, avg_min, avg_avg);
} // end of function

void base_8bit_sim(int items, int max_size) {

  unsigned long total_increments = 0;
  c_stats *c_stats_arr[NUM_ITER];

  for (int iter = 0; iter < NUM_ITER; iter++) {

    srand(time(NULL));

    // Stack-dynamic array to store the address access counters
    unsigned char ints[items];

    // Iterate through the array and set each counter to 0
    for (int i = 0; i < items; i++) {
      ints[i] = 0;
    }

    Bytef *compr;         // compression buffer
    uLongf compr_len = 0; // length of compression buffer
    int incr_cntr = 0;    // an address access incrementation counter

    // while the compression length is less than the maximum, then perform
    // another address access
    while (compr_len < max_size) {

      // perform increment
      int index = rand() % items;

      // if after increment it is 0, then it has overflowed
      if ((unsigned char)(ints[index] + 1) == 0) {
        // printf("\nchar(%i, %iB)[%i] : (%i accesses) : the data type has "
        //        "overflowed at index = (%i)!\n",
        //        items, max_size, iter, incr_cntr, index);
        goto end_char;
      }

      ints[index] += 1;

      // perform compression
      uLong input_len =
          sizeof(unsigned char) * items + 1; // Include the null terminator
      Bytef *compr = (Bytef *)malloc(
          input_len * 2); // Allocate memory for compression buffer
      uLongf compr_len = input_len * 2; // Size of the compression buffer

      int ret = compress(compr, &compr_len, (const Bytef *)ints, input_len);

      if (ret == Z_OK) {

        incr_cntr++;

        if (compr_len >= max_size) {
        end_char:
          // printf("\nchar(%i, %iB)[%i] : (%i accesses) : Current Compression "
          //        "Size = %lu",
          //        items, max_size, iter, incr_cntr, compr_len);
          free(compr);
          total_increments += incr_cntr;
          c_stats *stats = get_stats_c(ints, items);
          c_stats_arr[iter] = stats;
          break;
        }

      } else {
        ints[index]--;
        fprintf(
            stderr,
            "\nbase_8bit(%i, %iB)[%i] : (%i accesses) : Compression failed\n",
            items, max_size, iter, incr_cntr);
      }
    }
  }

  printf("\nbase_8bit(%i, %iB) : Average Increments per Iteration = %lu\n",
         items, max_size, total_increments / NUM_ITER);

  unsigned long long avg_med = 0;
  unsigned long long avg_max = 0;
  unsigned long long avg_min = 0;
  unsigned long long avg_avg = 0;

  for (int i = 0; i < NUM_ITER; i++) {
    avg_med += c_stats_arr[i]->med;
    avg_max += c_stats_arr[i]->max;
    avg_min += c_stats_arr[i]->min;
    avg_avg += c_stats_arr[i]->avg;
  }

  avg_med /= NUM_ITER;
  avg_max /= NUM_ITER;
  avg_min /= NUM_ITER;
  avg_avg /= NUM_ITER;

  printf("\nAVG MED := %llu\nAVG_MAX := %llu\nAVG_MIN := "
         "%llu\nAVG_AVG := %llu\n\n",
         avg_med, avg_max, avg_min, avg_avg);
}

void base_16bit_sim(int items, int max_size) {
  unsigned long total_increments = 0;
  s_stats *s_stats_arr[NUM_ITER];

  for (int iter = 0; iter < NUM_ITER; iter++) {
    // Seed the random number generator
    srand(time(NULL));

    // Stack-dynamic array to store the address access counters
    unsigned short ints[items];

    // Iterate through the array and set each counter to 0
    for (int i = 0; i < items; i++) {
      ints[i] = 0;
    }

    Bytef *compr;         // compression buffer
    uLongf compr_len = 0; // length of compression buffer
    int incr_cntr = 0;    // an address access incrementation counter

    // while the compression length is less than the maximum, then perform
    // another address access
    while (compr_len < max_size) {

      // perform increment
      int index = rand() % items;
      if ((unsigned short)(ints[index] + 1) == 0) {
        // printf("\nshort(%i, %iB)[%i] : (%i accesses) : the data type has "
        //        "overflowed at index = (%i)!\n",
        //        items, max_size, iter, incr_cntr, index);
        goto end_s;
      }
      ints[index] += 1;

      // perform compression
      uLong input_len =
          sizeof(unsigned short) * items + 1; // Include the null terminator
      Bytef *compr = (Bytef *)malloc(
          input_len * 2); // Allocate memory for compression buffer
      uLongf compr_len = input_len * 2; // Size of the compression buffer

      int ret = compress(compr, &compr_len, (const Bytef *)ints, input_len);

      if (ret == Z_OK) {

        incr_cntr++;

        if (compr_len >= max_size) {
        end_s:
          // printf("\nshort(%i, %iB)[%i] : (%i accesses) : Current Compression
          // "
          //        "Size = %lu",
          //        items, max_size, iter, incr_cntr, compr_len);
          free(compr);
          total_increments += incr_cntr;
          s_stats *stats = get_stats_s(ints, items);
          s_stats_arr[iter] = stats;
          break;
        }

      } else {
        ints[index]--;
        fprintf(
            stderr,
            "\nbase_16bit(%i, %iB)[%i] : (%i accesses) : Compression failed\n",
            items, max_size, iter, incr_cntr);
      }
    }
  }

  printf("\nbase_16bit(%i, %iB) : Average Increments per Iteration = %lu\n",
         items, max_size, total_increments / NUM_ITER);

  unsigned long long avg_med = 0;
  unsigned long long avg_max = 0;
  unsigned long long avg_min = 0;
  unsigned long long avg_avg = 0;

  for (int i = 0; i < NUM_ITER; i++) {
    avg_med += (unsigned long long)s_stats_arr[i]->med;
    avg_max += (unsigned long long)s_stats_arr[i]->max;
    avg_min += (unsigned long long)s_stats_arr[i]->min;
    avg_avg += (unsigned long long)s_stats_arr[i]->avg;
  }

  avg_med /= NUM_ITER;
  avg_max /= NUM_ITER;
  avg_min /= NUM_ITER;
  avg_avg /= NUM_ITER;

  printf("\nAVG MED := %llu\nAVG_MAX := %llu\nAVG_MIN := "
         "%llu\nAVG_AVG := %llu\n\n",
         avg_med, avg_max, avg_min, avg_avg);
}

int main() {

  base_8bit_sim(LO_ITEM, LO_MAX);
  base_16bit_sim(LO_ITEM, LO_MAX);
  split_9bit_sim(LO_ITEM, LO_MAX);
  split_10bit_sim(LO_ITEM, LO_MAX);
  split_16bit_sim(LO_ITEM, LO_MAX);

  base_8bit_sim(HI_ITEM, HI_MAX);
  base_16bit_sim(HI_ITEM, HI_MAX);
  split_9bit_sim(HI_ITEM, HI_MAX);
  split_10bit_sim(HI_ITEM, HI_MAX);
  split_16bit_sim(HI_ITEM, HI_MAX);

  return 0;
}
