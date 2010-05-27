/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef VBOOT_REFERENCE_QSORT_H_
#define VBOOT_REFERENCE_QSORT_H_

#define MAX_QUICK_SORT_ELEMENT_SIZE 512

/* QuickSort() is an in-place, and unstable quick sort implementation.
 * Given a compare function, this function sorts elements for you.
 * 'elements' points to the base of un-sorted array.
 * 'number_of_elements' indicates the number of elements in array.
 * 'size' indicates the size (in bytes) of an element unit.
 * The 'compare_function' should return true if 'a' precedes 'b'.
 *
 * NOTE: For performance issue, we reserve a static buffer for swap.
 *       So that the 'size' cannot exceed the buffer size
 *       (MAX_QUICK_SORT_ELEMENT_SIZE). */
void QuickSort(void *elements, int number_of_elements, int size,
               int (*compare_function)(const void *a, const void *b));

#endif  /* VBOOT_REFERENCE_QSORT_H_ */
