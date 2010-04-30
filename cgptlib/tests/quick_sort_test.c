/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "quick_sort_test.h"
#include "cgpt_test.h"
#include "quick_sort.h"
#include "utility.h"

#define MAX_NUMBER_OF_TEST_ELEMENTS 16

/* callback function for QuickSort.
 * To get ascent results, this function returns 1 if a < b.
 * Returns 0 if a >= b. */
int ascent_compare(const void *a_, const void *b_) {
  const int *a = a_;
  const int *b = b_;
  if (*a < *b) return 1;
  return 0;
}

/* Used to verify if an array is sorted as ascent which means
 * 'a' (previous element) is smaller than 'b' (back element).
 * Returns 1 for ture, 0 for false. */
int ascent_verify(const int a, const int b) {
  return (a <= b) ? 1 : 0;
}

/* callback function for QuickSort.
 * To get descent results, this function returns 1 if a > b.
 * Returns 0 if a <= b. */
int descent_compare(const void *a_, const void *b_) {
  const int *a = a_;
  const int *b = b_;
  if (*a > *b) return 1;
  return 0;
}

/* Used to verify if an array is sorted as descent which means
 * 'a' (previous element) is lager than 'b' (back element).
 * Returns 1 for ture, 0 for false. */
int descent_verify(const int a, const int b) {
  return (a >= b) ? 1 : 0;
}

/* We provide 2 ways to sort the array. One for ascent, and another for descent.
 */
struct {
  int (*compare)(const void *a, const void *b);
  int (*verify)(const int a, const int b);
} directions[] = {
  { ascent_compare, ascent_verify, },
  { descent_compare, descent_verify, },
};

/* Here are the fixed patterns to test. Especially those corner cases that
 * random test cannot easily reproduce. */
struct {
  int number;  /* number of integers saved in array */
  int unsorted[MAX_NUMBER_OF_TEST_ELEMENTS];
} test_data[] = {
  {0, {}, },
  {1, {0, }, },
  {2, {1, 1,}, },
  {2, {1, 2,}, },
  {2, {2, 1,}, },
  {3, {1, 3, 2}, },
  {3, {2, 1, 3}, },
  {4, {1, 1, 3, 2}, },
  {4, {3, 1, 2, 2}, },
  {4, {1, 3, 3, 2}, },
  {5, {1, 2, 3, 4, 5}, },
  {5, {5, 5, 5, 3, 3}, },
  {5, {5, 1, 3, 2, 4}, },
  {5, {4, 5, 2, 3, 1}, },
  {6, {5, 4, 3, 2, 1, 6}, },
  {7, {5, 4, 3, 2, 1, 6, 7}, },
  {7, {2, 5, 4, 6, 7, 1, 3}, },
  {7, {7, 6, 1, 5, 3, 4, 2}, },
};

int TestQuickSortFixed() {
  int data;
  int dir;
  int sorted[MAX_NUMBER_OF_TEST_ELEMENTS];

  for (dir = 0; dir < ARRAY_SIZE(directions); ++dir) {
    for (data = 0; data < ARRAY_SIZE(test_data); ++data) {
      int i;
      for (i = 0; i < test_data[data].number; ++i)
        sorted[i] = test_data[data].unsorted[i];

      QuickSort(sorted, test_data[data].number, sizeof(int),
                directions[dir].compare);

      for (i = 0; i < test_data[data].number - 1; ++i)
        EXPECT(directions[dir].verify(sorted[i], sorted[i + 1]));
    }
  }

  return TEST_OK;
}

/* Random test. We don't really need a truely random test. A pseudo-random
 * pattern with large 'try_num' is good enough to test.
 */
static uint32_t Random() {
  static uint32_t seed = 0x600613;  /* 'GOOGLE' :-) */
  return (seed = seed * 701 + 179);
}

int TestQuickSortRandom() {
  int try_num;
  int i, dir;

  for (dir = 0; dir < ARRAY_SIZE(directions); ++dir) {
    for (try_num = 100000; try_num > 0; --try_num) {
      int number_of_elements;
      int *p;

      number_of_elements = Random() % 181;
      p = Malloc(sizeof(int) * number_of_elements);
      for (i = 0; i < number_of_elements; ++i)
        p[i] = Random() % 173;

      QuickSort(p, number_of_elements, sizeof(int), directions[dir].compare);

      for (i = 0; i < number_of_elements - 1; ++i)
        EXPECT(directions[dir].verify(p[i], p[i + 1]));

      Free(p);
    }
  }

  return TEST_OK;
}
