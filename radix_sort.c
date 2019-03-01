#include <stdio.h>
#include <math.h>
#include <time.h>

#define BASE_BITS 8
#define BASE (1 << BASE_BITS)
#define MASK (BASE-1)
#define DIGITS(v, shift) (((v) >> shift) & MASK)
#define SEED 13516085

int number_of_int;
int *array_not_sorted1, *array_not_sorted2, *array_not_sorted3;
clock_t start_time_serial, end_time_serial;
clock_t start_time_parallel, end_time_parallel;
int* count_result[100];
int global_count[100][10];

void create_random_array(int* arr, int n) {
	srand(SEED);
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

void count_sort_parallel(int* arr, int n, int index_digit) {
	int* output = (int*) malloc(n*sizeof(int)); 
	int exp = pow(10, index_digit);

	for(int i = n-1; i >= 0; i--) {
		output[global_count[index_digit][(arr[i]/exp)%10] - 1] = arr[i];
		global_count[index_digit][(arr[i]/exp)%10]--;
	}

	memcpy(arr, output, n*sizeof(int));
}


int get_number_digit(int n) {
	int result = 0;
	while(n > 0) {
		result++;
		n /= 10;
	}
	return result;
}

void fill_count(int* arr, int n) {
	int local_count[10] = {0};

	int rank = omp_get_thread_num();
	int exp = pow(10, rank);

	#pragma omp parallel for reduction(+:local_count[:10])
	for (int i = 0; i < n; i++) {
		local_count[(arr[i]/exp)%10]++;
	}

	for(int i = 1; i < 10; i++) {
		local_count[i] += local_count[i-1];
	}

	memcpy(global_count[rank], local_count, 10*sizeof(int)); 
}

void radix_sort_parallel(int* arr, int n) {
	int m = get_max(arr, n);
	int number_digit = get_number_digit(m);

	#pragma omp parallel num_threads(number_digit)
	fill_count(arr, n);

	for(int i = 0; i < number_digit; i++) {
		count_sort_parallel(arr, n, i);
	}

	return;
}

int check_sorted(int* array_sorted, int* array_test, int n) {
	for(int i = 0; i < n; i++) {
		if (array_sorted[i] != array_test[i]) {
			return 0;
		}
	}
	return 1;
}

void print_array(int* arr, int n) {
	for(int i = 0; i < n; i++) {
		printf("%d\n", arr[i]);
	}
}

int main(int argc, char *argv[]) {
	int number_of_int = strtol(argv[1], NULL, 10);
	int success;

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
	// start_time_parallel = clock();
	// omp_lsd_radix_sort(number_of_int, array_not_sorted2);
	// end_time_parallel = clock();

	// success = check_sorted(array_not_sorted1, array_not_sorted2, number_of_int);
	// if (success == 1) {
	// 	printf("Array Sorted\n");
	// }

	// printf("Waktu yang dibutuhkan untuk Radix Sort Parallel Haichuanwan: %lfms\n", (double)(end_time_parallel - start_time_parallel)/CLOCKS_PER_SEC * 1000 * 1000);

	start_time_parallel = clock();
	radix_sort_parallel(array_not_sorted3, number_of_int);
	end_time_parallel = clock();

	success = check_sorted(array_not_sorted1, array_not_sorted3, number_of_int);
	if (success == 1) {
		printf("Array Sorted\n");
	} 

	printf("Waktu yang dibutuhkan untuk Radix Sort Parallel Saya: %lfms\n", (double)(end_time_parallel - start_time_parallel)/CLOCKS_PER_SEC * 1000 * 1000);
}