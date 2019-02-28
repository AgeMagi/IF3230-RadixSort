#include <stdio.h>
#include <math.h>
#include <time.h>

#define BASE_BITS 8
#define BASE (1 << BASE_BITS)
#define MASK (BASE-1)
#define DIGITS(v, shift) (((v) >> shift) & MASK)

int number_of_int;
int *array_not_sorted1, *array_not_sorted2, *array_not_sorted3;
clock_t start_time_serial, end_time_serial;
clock_t start_time_parallel, end_time_parallel;
int* count_result[100];

void create_random_array(int* arr, int n) {
	int seed = 13516085;
	srand(seed);
	for (int i = 0; i < n; i++) {
		arr[i] = (int)rand();
	}
}

int get_max(int* arr, int n) {
	int mx = arr[0]; 
    for (int i = 1; i < n; i++) 
        if (arr[i] > mx) 
            mx = arr[i]; 
    return mx; 
}

void count_sort(int* arr, int n, int exp) {
	int* output;
	int i, count[10] = {0};

	output = (int*) malloc(n*sizeof(int));
	for(i = 0; i < n; i++) {
		count[(arr[i]/exp)%10]++;
	}

	for(i = 1; i < 10; i++) {
		count[i] += count[i-1];
	}

	for(i = n - 1; i >= 0; i--) {
		output[count[(arr[i]/exp)%10] - 1] = arr[i];
		count[(arr[i]/exp)%10]--;
	}

	for(i = 0; i < n; i++) {
		arr[i] = output[i];
	}
}

void radix_sort_serial(int* arr, int n) {
	int m = get_max(arr, n);
	for (int exp = 1; m/exp > 0; exp *= 10) {
		count_sort(arr, n, exp);
	}
}

void count_sort_parallel(int* arr, int n, int exp) {
	int* output;
	int i, count[10] = {0};

	output = (int*) malloc(n*sizeof(int));

	#pragma omp parallel for reduction(+:count[:10])
	for(int i = 0; i < n; i++) {
		count[(arr[i]/exp)%10]++;
	}

	for(i = 1; i < 10; i++) {
		count[i] += count[i-1];
	}

	for(i = n - 1; i >= 0; i--) {
		output[count[(arr[i]/exp)%10] - 1] = arr[i];
		count[(arr[i]/exp)%10]--;
	}

	for(i = 0; i < n; i++) {
		arr[i] = output[i];
	}
}

void radix_sort_parallel(int* arr, int n) {
	int m = get_max(arr, n);

	for(int exp = 1; m/exp > 0; exp *= 10) {
		count_sort_parallel(arr, n, exp);
	}

	return;
}

void omp_lsd_radix_sort(int n, int* arr) {
    int * buffer = malloc(n*sizeof(int));
    int total_digits = sizeof(int)*8;
 
    //Each thread use local_bucket to move data
    int i;
    for(int shift = 0; shift < total_digits; shift+=BASE_BITS) {
        int bucket[BASE] = {0};
 
        int local_bucket[BASE] = {0}; // size needed in each bucket/thread
        //1st pass, scan whole and check the count
        #pragma omp parallel firstprivate(local_bucket)
        {
            #pragma omp for schedule(static) nowait
            for(i = 0; i < n; i++){
                local_bucket[DIGITS(arr[i], shift)]++;
            }
            #pragma omp critical
            for(i = 0; i < BASE; i++) {
                bucket[i] += local_bucket[i];
            }
            #pragma omp barrier
            #pragma omp single
            for (i = 1; i < BASE; i++) {
                bucket[i] += bucket[i - 1];
            }
            int nthreads = omp_get_num_threads();
            int tid = omp_get_thread_num();
            for(int cur_t = nthreads - 1; cur_t >= 0; cur_t--) {
                if(cur_t == tid) {
                    for(i = 0; i < BASE; i++) {
                        bucket[i] -= local_bucket[i];
                        local_bucket[i] = bucket[i];
                    }
                } else { //just do barrier
                    #pragma omp barrier
                }
 
            }
            #pragma omp for schedule(static)
            for(i = 0; i < n; i++) { //note here the end condition
                buffer[local_bucket[DIGITS(arr[i], shift)]++] = arr[i];
            }
        }
        //now move data
        int* tmp = arr;
        arr = buffer;
        buffer = tmp;
    }
    free(buffer);
}

int check_sorted(int* array_sorted, int* array_test, int n) {
	for(int i = 0; i < n; i++) {
		if (array_sorted[i] != array_test[i]) {
			return 0;
		}
	}

	return 1;
}

int main(int argc, char *argv[]) {
	int number_of_int = strtol(argv[1], NULL, 10);
	
	array_not_sorted1 = (int*) malloc(number_of_int*sizeof(int));
	array_not_sorted2 = (int*) malloc(number_of_int*sizeof(int));
	array_not_sorted3 = (int*) malloc(number_of_int*sizeof(int));

	create_random_array(array_not_sorted1, number_of_int);
	create_random_array(array_not_sorted2, number_of_int);
	create_random_array(array_not_sorted3, number_of_int);

	start_time_serial = clock();
	radix_sort_serial(array_not_sorted1, number_of_int);
	end_time_serial = clock();

	printf("Waktu yang dibutuhkan untuk Radix Sort Serial: %lfms\n", (double)(end_time_serial - start_time_serial)/CLOCKS_PER_SEC * 1000 * 1000);

	start_time_parallel = clock();
	omp_lsd_radix_sort(number_of_int, array_not_sorted2);
	end_time_parallel = clock();

	int success = check_sorted(array_not_sorted1, array_not_sorted2, number_of_int);
	if (success == 1) {
		printf("Array Sorted\n");
	}

	printf("Waktu yang dibutuhkan untuk Radix Sort Parallel Haichuanwan: %lfms\n", (double)(end_time_parallel - start_time_parallel)/CLOCKS_PER_SEC * 1000 * 1000);

	start_time_parallel = clock();
	radix_sort_parallel(array_not_sorted3, number_of_int);
	end_time_parallel = clock();

	success = check_sorted(array_not_sorted1, array_not_sorted3, number_of_int);
	if (success == 1) {
		printf("Array Sorted\n");
	}

	printf("Waktu yang dibutuhkan untuk Radix Sort Parallel Saya: %lfms\n", (double)(end_time_parallel - start_time_parallel)/CLOCKS_PER_SEC * 1000 * 1000);
}