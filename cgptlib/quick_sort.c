/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "quick_sort.h"
#include <stdint.h>
#include "utility.h"

/* Make sure we won't overflow the buffer. */
#define SAFE_SIZE(size) \
    ((size > MAX_QUICK_SORT_ELEMENT_SIZE) ? MAX_QUICK_SORT_ELEMENT_SIZE : size)

/* Swap a and b. Since the type of 'a' and 'b' are unknown, caller must indicate
 * 'size' bytes. */
void Swap(void *a, void *b, int size) {
  static uint8_t buffer[MAX_QUICK_SORT_ELEMENT_SIZE];
  Memcpy(buffer, a, SAFE_SIZE(size));
  Memcpy(a, b, SAFE_SIZE(size));
  Memcpy(b, buffer, SAFE_SIZE(size));
}

/* Given a pivot and left/right boundary, this function returns a new pivot,
 * and ensure the elements in the left-side of pivot are preceding elements,
 * and elements in the right-side of pivot are following elements. */
static int
Partition(int (*compare_function)(const void *a, const void *b),
          void *vp_elements, int size, int left, int right, int pivot) {
  uint8_t *elements = vp_elements;
  int i;

  Swap(&elements[right * size], &elements[pivot * size], size);
  for (i = left; i < right; ++i) {
    if (compare_function(&elements[i * size], &elements[right * size])) {
      Swap(&elements[left * size], &elements[i * size], size);
      ++left;
    }
  }
  Swap(&elements[left * size], &elements[right * size], size);
  return left;
}

/* Given left and right boundary, this function returns a sorted elements in
 * that range. */
static void
RecursiveQuickSort(void *elements, int left, int right, int size,
                   int (*compare_function)(const void *a, const void *b)) {
  int pivot;

  if (right <= left) return;
  pivot = (left + right) / 2;
  pivot = Partition(compare_function, elements, size, left, right, pivot);
  RecursiveQuickSort(elements, left, pivot - 1, size, compare_function);
  RecursiveQuickSort(elements, pivot + 1, right, size, compare_function);
}

void QuickSort(void *elements, int number_of_elements, int size,
               int (*compare_function)(const void *a, const void *b)) {
  RecursiveQuickSort(elements, 0, number_of_elements - 1, size, compare_function);
}
