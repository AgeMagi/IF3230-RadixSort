#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

void Hello(void); /* Thread function */

int main(int argc, char *argv[]) {
   int thread_count = strtol(argv[1], NULL, 10);
  printf("%d\n", thread_count);

   #pragma omp parallel num_threads(thread_count)
   Hello();

   for(int i = 1; i < 10; i++) {
      printf("%d HAI\n", i);
   }

   return 0;
}

void Hello(void) {
   int my_rank = omp_get_thread_num();
   int thread_count = omp_get_num_threads();

   printf("Hello from thread %d of %d\n", my_rank, thread_count);
}
