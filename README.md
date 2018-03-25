# Segmented-Paging

**Names:** Liam Davies, Kevin Lee, Brian Ellsworth <br>
**NetIDs:** lmd312, kjl156, bje40 <br>
**ilab machine:** factory.cs.rutgers.edu


# Structure
---
## Memory
Our 8mb of Memory is structured as follows:

![alt text](/Graphics/mem.png)

* ### Specifications
  * Designed and tested on *factory.cs.rutgers.edu*
  * **Total Memory Space:** 8,388,608 bytes (*8 mb*)
  * **Page Size:** 4,096 bytes
  * **Number of Pages:** 2048 Pages

* ### Page Types
  * **Thread** - the pages used when malloc is called in any given thread
    * *NOTE:* These are the only ones swapped using segmentation faults
  * **Shalloc** - the pages used when shalloc is called
  * **Library** - the pages used when the pthread library calls malloc
  * **File Table** - the pages used to store the page table representing the pages in the swapfile
  * **Mem Table** - the pages used to store the page table representing the thread pages in memory

* ### Number of Pages
  * **Thread** - 1020 Pages *(4,177,920 bytes)*
  * **Shalloc** - 4 Pages *(16,384 bytes)*
  * **Library** - 1014 Pages *(4,153,344 bytes)*
  * **File Table** - 8 Pages *(32,768 bytes)*
  * **Mem Table** - 2 Pages *(8,192 bytes)*

## Malloc/Free

Our malloc/free metadata structure is as follows:

![alt text](/Graphics/malloc.png)

* ### Structure:
  * *Code:*
  ```c
  typedef struct metaBlock {
    struct metaBlock * next;
    uint size;
  } mb;
  ```

  * A **front** pointer points to the first allocated block's metadata chunk
  * Each block stores its **size** and a pointer (**next**) to the next block's metadata chunk

* ### Mallocing:
  * Finds open space using a *"first free"* style algorithm
  * Subject to *internal fragmentation*
  * Will return *NULL* if not enough space remains

* ### Freeing:
  * Finds the pointer in the list and removes it from the metadata linked list
    * *NOTE:* Pointers that have not been malloc'd cannot be freed, however, attempting to do so will cause an error, not a crash

## Page Tables

Our page table consists of  an array of variables of the structure type below:
```c
typedef struct pageInfo {
  uint tid;
  uint index;
} pageInfo;
```

* ### Variables:
  * **tid** - the thread id of the thread that this page belongs to
  * **index** - the index of the page for the given thread id *(where does it belong in the page ordering)*

* ### Usage:
  * There are two page tables for this library:
    1. **Thread Memory Page Table** - m_front
    2. **Swap File Page Table** - f_front
  * The index of the pageInfo is directly related to the index of the page in the respective space
    * *e.g:* m_front[5] is the metadata for what is currently in the 5th page of the *Thread Memory Page Table*

# Algorithms and Specification Notes
---
## Page Swapping

* ### On Segfault:
  1. Determine which thread attempted access.
  2. Determine where the access was.
    * If the access was outside of our memory space, let the segfault through. *(Skip remaining steps)*
  3. Determine the index of the page that the segfault occured on.
  4. Determine if that thread has a page with that index.
    * If they don't, then let the segfault through. *(Skip remaining steps)*
  5. Swap the correct data into the spot and un-mprotect that space.

* ### Choosing a Victim:
  * We **did not** implement a non-naive victim selection algorithm
  * The thread currently occupying the space that the page needs to be swapped into, will be swapped into the swapfile

* ### To see full code:
  * Look for:
  ```c
  static void seghandler(int sig, siginfo_t * si, void * unused) {...};
  ```
  in *SegPage.c*

## Specifications:
* ### Limitations:
  * **62 Threads** per program - *MAX*
  * **64,000 bytes** per stack frame - *MAX* *(Going over will cause undefined behavior)*
  * **4,177,920 bytes** are available for malloced space and metadata per thread - *MAX*

* ### Compliation:
  * We have created our malloc library as another header file, **SegPage_t.h**, that is required to use my_pthread.
  * In order to compile a program that uses both, please look at our Makefile for example compliations

# Tests
---
## Test1
* ### Purpose
  * Used to test if malloc works for the scheduler without any threads involved.

* ### Operations:
  * Creates 5 threads
  * Passes them references to main's stack
  * Each function adds a different number to that reference
  * Exits and joins all threads
  * Prints out the references

* ### Upon Success:
    * It will print:
    ```
    i1: 1, i2: 2, i3: 3, i4: 1, i5: 2
    ```

## Test2
* ### Purpose
  * Used to stress test page swapping in memory only *(no swap file)*

* ### Operations:
  * Creates 2 threads
  * Each thread mallocs (an int or a struct) 1000 times
  * After each malloc, the thread yields, causing constant thrashing
  * Each thread then frees all of the pointers they allocated,
      indicating to main if the value is the same as what they set it to
  * Main then prints the indicators and exits

* ### Upon Success:
  * It will print:
  ```
  Complete.
  i1: 1, i2: 1
  ```

## Test3
* **NOTE:** Takes a LONG time to complete, lots of thrashing and I/O.
  * Can take anywhere between 30 seconds to a few minutes depending on the ilab machine's workload

* ### Purpose
  * Used to stress test page swapping in memory and the swapfile

* ### Operations:
  * Creates threads until no more threads can be made (and records the number)
  * Each thread mallocs (20 ints) 1000 times
  * After each malloc, the thread yields, causing constant thrashing
  * Each thread then frees all of the pointers they allocated,
      indicating to main if the value is the same as what they set it to
  * Main then prints Success and the number of threads created

* ### Upon Success:
  * It will print:
  ```
  Success. Created 62 threads.
  ```

## Test4
* ### Purpose
  * Used to test page swapping in the swapfile

* ### Operations:
  * Creates 2 threads
  * Each thread mallocs 4,177,000 bytes
  * Yields are used to ensure the swap file is used
  * Each thread sets some variables in the space
  * Each thread then frees the pointer it allocated,
      indicating to main if the value is the same as what it set it to
  * Main then prints Success

* ### Upon Success:
  * It will print:
  ```
  Success.
  ```

## Test4
* ### Purpose
  * Used to test shalloc

* ### Operations:
  * Creates 1 thread
  * The thread shallocs, sets the variable, then exits, returning the pointer
  * Main prints the variable, then frees the space

* ### Upon Success:
  * It will print:
  ```
  Complete. - 5
  ```
