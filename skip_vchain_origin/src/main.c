#include "sim.h"

void *do_query_range_query(void * arg)
{
	long key, data, rv_data;
	int i;
	int op_num = *((int*)arg);

	for(i=0; i< op_num/100 ;i++){
		key = (long) rand() % (MAX_KEY/3);
		long next_count= (long) rand() % 100;  
		skip_cnt+=skip_list_range_query(key ,next_count, sl);
		//DEBUG_PRINT(("tid = %ld prcessed %i ops\n", syscall(SYS_gettid), i));
	
	}	
	return (void*) (arg+1);
}

void *do_query_get(void* arg)
{
	long key, data, rv_data;
	int i;
	int op_num = *((int*)arg);

	for(i=0; i< op_num; i++){
		key = (long) rand() % MAX_KEY;
		get_cnt++;
		rv_data=skip_list_get(key, sl);
	}
	return (void*) (arg+1);
}
void *do_query_put(void* arg)
{
	long key, data, rv_data;
	int i;
	int op_num = *((int*)arg);
	DEBUG_PRINT(("tid %ld is created\n", syscall(SYS_gettid)));

	key = 0;

	for(i=0; i < op_num; i++){
#ifndef CSKIP_LIST
		pthread_mutex_lock(&sl->list_lock);
#endif
		//key = key_box[i];
		key = (long) rand() % MAX_KEY;
		data = (long) rand() % MAX_DATA;  
//		if (key % 2) {
		put_cnt++;
		skip_list_put(key, data, sl);
#ifndef CSKIP_LIST
		pthread_mutex_unlock(&sl->list_lock);
#endif
	
		DEBUG_PRINT(("tid = %ld prcessed %i ops\n", syscall(SYS_gettid), i));
	}

	return (void*) (arg+1);
}



int main(int argc, char* argv[])
{

	int i;
	int j;
	int rc; 
	//int* rv[MAX_THREAD_NUM]; 

	if(argc < 4) {
		printf("Usage: ./skip 16 10000 1 \n");
		printf("(thread_num op_num put(1)/get(2))\n");
		return -1;
	}
	
	int nthreads = atoi(argv[1]);
	int* rv = (int*) malloc (sizeof(int) * nthreads);
	int op_num = atoi(argv[2]);
	long key_max = (long)(op_num * 10);
	key_box=(int *) malloc (sizeof(int) * op_num);	
	int benchmark = atoi(argv[3]);
	FILE* fp;
	double time;



	// initialize skip_list 

	skip_list_init(&sl);
#ifndef CSKIP_LIST
	if(sl) pthread_mutex_init(&(sl->list_lock), NULL);
#endif
#if 1	
	// create permutation
	for( i=0; i< op_num ; i++){//중복되지 않은 key값 생성
		key_box[i]= (long) rand() % key_max;
		for(j=0; j < i ; j++){
			if(key_box[j] == key_box[i]){
				i--;
				break;
			}
		}
	}
#endif 
#if 0
	i=0;
	fp=fopen("/home/js/skip_vchain/trc/trc_10000", "r");
	if(fp==NULL){
		printf("open error ");
		return -1 ;
	}else{
		while(0 < fscanf(fp,"%d\n", &key_box[i])){
			i++;	
		}
	}
	fclose(fp);	
#endif
	if(benchmark==1){

		
		// start time 
		if(gettimeofday(&t_start, NULL) == -1){
			printf("failed to read time\n");
			return -1;	
		}
		// put threads 
		for ( i=0; i < nthreads; i++){
			Pthread_create(&threads[i], NULL, do_query_put, (void*) &op_num);
		}
		// wait threads 
		for ( i=0; i < nthreads; i++){
			Pthread_join(threads[i], (void **) &rv[i]);
		}
		// end time 
		if(gettimeofday(&t_end, NULL) == -1){
			printf("failed to read time\n");
			return -1;	
		}
		// print list 
		skip_list_print_all (sl);
		print_result(op_num * nthreads);

	}else if(benchmark==2){
		
		// put threads 
		for ( i=0; i < nthreads; i++){
			Pthread_create(&threads[i], NULL, do_query_put, (void*) &op_num);
		}

		
		// wait put threads 
		for ( i=0; i < nthreads; i++){
			Pthread_join(threads[i], (void **) &rv[i]);
		}
	
		// start time 
		if(gettimeofday(&t_start, NULL) == -1){
			printf("failed to read time\n");
			return -1;	
		}
		
		// get threads 
		for ( i=0; i < nthreads; i++){
			Pthread_create(&threads[i], NULL, do_query_get, (void*) &op_num);
		}
		// wait threads 
		for ( i=0; i < nthreads; i++){
			Pthread_join(threads[i], (void **) &rv[i]);
		}
		// end time 
		if(gettimeofday(&t_end, NULL) == -1){
			printf("failed to read time\n");
			return -1;	
		}
		//time=(double)(t_end.tv_sec)+(double)(t_end.tv_usec)/1000000.0-(double)(t_start.tv_sec)-(double)(t_start.tv_usec)/1000000.0;
		//printf("elase time : %6.4f\n", time);
		// print list 
		skip_list_print_all (sl);
		print_result(op_num * nthreads);

	}
	else if(benchmark==3){
		// put threads 
		for ( i=0; i < nthreads; i++){
			Pthread_create(&threads[i], NULL, do_query_put, (void*) &op_num);
		}
		
		// wait put threads 
		for ( i=0; i < nthreads; i++){
			Pthread_join(threads[i], (void **) &rv[i]);
		}
		// start time 
		if(gettimeofday(&t_start, NULL) == -1){
			printf("failed to read time\n");
			return -1;	
		}
		//printf("put done\n ");	
		// get threads 
		for ( i=0; i < nthreads; i++){
			Pthread_create(&threads[i], NULL, do_query_range_query, (void*) &op_num);
		}
		// wait threads 
		for ( i=0; i < nthreads; i++){
			Pthread_join(threads[i], (void **) &rv[i]);
		}
		// end time 
		if(gettimeofday(&t_end, NULL) == -1){
			printf("failed to read time\n");
			return -1;	
		}
		// print list 
		skip_list_print_all (sl);
		print_result(op_num * nthreads /100);
	}
	else{
		printf("invalid benchamrk number \n");
		return -1;
	}

	// destroy skip_list
	skip_list_destroy(sl);

}
