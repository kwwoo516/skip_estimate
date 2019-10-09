//#define PSKIP_LIST

#ifdef DEBUG
#define DEBUG_PRINT(x) printf x
#else
#define DEBUG_PRINT(x) do {} while (0)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>

#include "skip.h"
#include "thread.h"

#define _GNU_SOURCE
#define MAX_THREAD_NUM 128

#define MAX_KEY 50000 // 2^15
#define MAX_DATA 65536 // 2^15
#define MAX_OP_NUM 10000

pthread_t threads[MAX_THREAD_NUM];

typedef struct _ret_t{
	int ops;
	int* latency;
} args_t;

struct timeval t_start;
struct timeval t_end;

atomic_long put_cnt = 0;
atomic_long get_cnt = 0;
atomic_int skip_cnt=0;

struct skip_list_t* sl;
int* key_box;

void print_result(int ops)
{
/*
#ifdef PSKIP_LIST 
	printf("pskip ");
#else
	printf("skip ");
#endif
*/
/*
	printf("put %ld get %ld time(s) %6.4f iops %6.2f\n",
		put_cnt,
		get_cnt,
		t_end.tv_sec - t_start.tv_sec + (t_end.tv_usec - t_start.tv_usec) / 1000000.0, 
		ops / (t_end.tv_sec - t_start.tv_sec + (t_end.tv_usec - t_start.tv_usec) / 1000000.0));
*/
	printf("%6.2f\n" ,ops / (t_end.tv_sec - t_start.tv_sec + (t_end.tv_usec - t_start.tv_usec) / 1000000.0));
	//printf("node count : %ld \n", sl->node_cnt);
	//printf("skipcount  : %d\n", skip_cnt);	
	return;
}
