#define MAX_LEVEL 12

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <stdatomic.h> 

#define TRUE 1
#define FALSE 0

#ifdef VALUE_CHAIN
struct vcnode_t{
	unsigned long t_stamp;
	long data;
	struct vcnode_t* next;
};
#endif

struct node_t {
	long key;
#ifdef VALUE_CHAIN
	struct vcnode_t* vchain; 
	long vchain_num;
#endif
	long data; //latest data
	//volatile atomic_long data_aptr;
	long level;
	struct node_t* next[MAX_LEVEL];
};

struct skip_list_t {
	struct node_t* list_head;
	atomic_long node_cnt;
	atomic_long max_height;
	long key_first;
	long key_last;
#ifdef VALUE_CHAIN
//	unsigned long chain_cnt;
	atomic_long chain_cnt;
#endif
	pthread_mutex_t list_lock;	
};

struct skip_iter{
	struct node_t * curr;
	long key;
	long data;
	unsigned long t_stamp;
	unsigned long skip_cnt;
};

int prob_tbl [MAX_LEVEL];
int fail_cnt;


static int get_level() {//random하게 level을 설정

	int rv, balancing, pivot; 
	balancing = 2;
	pivot = 1000 / balancing; 
	rv = 1;

	while(rv < pivot && rv < MAX_LEVEL && (rand() % 1000) < pivot) {
		rv++;
	}
	
	return rv;
}


#if 0
int CAS(Node_t **cur_node, Node_t *old_node, Node_t **new_node)
{
	int ret,f=0;
//	printf("1(In CAS) cur : %p, old : %p, new %p\n",*cur_node,old_node,*new_node);

	__asm__ __volatile__(  
            "LOCK\n\t"  
            "CMPXCHG %3, %0\n\t"
			"jnz DONE\n\t"
            "movl %4, %1\n\t " 
			"DONE:\n\t" 
            :"=m"(*cur_node),"=g"(ret)  
            :"a" (old_node),"r" (*new_node),"r"(f),"m"(*cur_node)  
            :"memory" 
            );

//	printf("2(In CAS) cur : %p, old : %p, new %p\n",*cur_node,old_node,*new_node);

//	printf("%d\n",ret);

	return ret;
}
#endif

extern int skip_list_init();
extern void skip_list_destroy();
extern int skip_list_put (long key, long data, struct skip_list_t* sl);
extern long skip_list_get (long key, struct skip_list_t* sl);
extern int skip_list_range_query (long key, long next_count, struct skip_list_t* sl);
extern void skip_list_print_all (struct skip_list_t* sl);

#if 0
extern int pskip_list_init();
extern void pskip_list_destroy();
extern int pskip_list_put (long key, long data, struct skip_list_t* sl);
extern long pskip_list_get (long key, struct skip_list_t* sl);
extern void pskip_list_print_all (struct skip_list_t* sl);
#endif
