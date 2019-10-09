#include "skip.h"

int level_count[MAX_LEVEL];

int skip_list_init(struct skip_list_t** sl) {

	int i;

	if(sl == NULL){
		printf("skip list pointer error \n");
		return -1;
	}

	// alloc skip_list 
	if (((*sl) = (struct skip_list_t*) malloc (sizeof(struct skip_list_t))) == NULL){
		fprintf(stderr, "Failed to malloc for skip_list");
		return -1;
	}
		
	if (((*sl)->list_head = (struct node_t*) malloc (sizeof(struct node_t))) == NULL){
		fprintf(stderr, "Failed to malloc for list_head");
		return -1;
	}

	// initialize list_head
	(*sl)->node_cnt = 0;
	(*sl)->list_head->key = -1;
	(*sl)->list_head->data = -1;
	//*((*sl)->list_head->data) = -1;
	//(*sl)->list_head->data = NULL;
	//pthread_mutex_init(&((*sl)->list_lock), NULL);

	for ( i=0; i < MAX_LEVEL; i++)
		(*sl)->list_head->next[i] = NULL;

	// initialize level_prob_table 
	for (i=0; i<MAX_LEVEL; i++){
		prob_tbl[i] = pow(2, MAX_LEVEL-1-i);
	}
	for (i=MAX_LEVEL-2; i>=0; i--){
		prob_tbl[i] = prob_tbl[i] + prob_tbl[i+1];
	}

	// init random function for level decision
	srand((unsigned) time (NULL));

	for(i=0; i < MAX_LEVEL; i++)
		level_count[i] = 0;

	fail_cnt = 0;

	return 0;
}


void skip_list_destroy(struct skip_list_t* sl) {

	struct node_t* curr;
	struct node_t* next;

	if(sl == NULL)
		return;

	curr = sl->list_head; 

	// free all nodes 
	while (curr) {
		next = curr->next[0]; // free node linked in lowest-level
		free(curr);
		curr = next;
	}

	free(sl);

	return;
}

long skip_list_get (long key, struct skip_list_t* sl) {
	
	struct node_t *curr, *prev, *nnode;
	int level, nlevel; 
	int i;

	if (sl == NULL){
		printf("skip list has null pointer");
		return -1;
	}

	// search position  
	level = MAX_LEVEL-1;
	prev = sl->list_head;

	while (level >= 0) { // search nodes in all levels
		curr = prev->next[level]; 
		while (curr) {
			if (key == curr->key) 
				return curr->data;
			if (key < curr->key) 
				break;
			prev = curr;
			curr = curr->next[level];  
		}
		// move to lower layar
		level--;
	}		
	assert (level == -1);
	assert (prev != NULL);

	return -1;
}


int pskip_find_level (long key, struct skip_list_t* sl, 
	struct node_t **back_ptr[], struct node_t *exp_ptr[], int low_level)
{
#ifdef DEBUG
	printf("pskip_find_level <== start \n");
#endif
	struct node_t *curr, *prev, *nnode;
	//struct node_t **back_ptr[MAX_LEVEL];
	//struct node_t *exp_ptr[MAX_LEVEL];
	long exp_data;

	int level, cnt;
	//pid_t pid;
	//pthread_t tid;
	uint64_t tid;

	if (sl == NULL || sl->list_head == NULL ){
		printf("skip list has null pointer");
		return -1;
	}

	//cnt = 0;

	// search position  
	level = MAX_LEVEL-1;
	prev = sl->list_head;

//	while (level >= 0) { // search nodes in all levels
	while (level >= low_level) { // search nodes in all levels
		curr = prev->next[level]; 
		while (curr) {

#ifdef DEBUG
//			pid_t tid = (pid_t) syscall (__NR_gettid); // for linux 
			//pid = getpid();
			//tid = pthread_self();
			pthread_threadid_np(NULL, &tid);
			printf("[%llu] curr = %ld\n", tid, curr->key); 
#endif
			if (key <= curr->key) 
				break;
			prev = curr;
			curr = curr->next[level];  
			//cnt++;
			//assert (cnt <= sl->node_cnt);
		}
		// tracking back pointers 
		back_ptr[level] = &prev->next[level];
		exp_ptr[level] = curr;

		// move to lower layar
		level--;
	}
	//assert (level == -1);

#if 0
	if (curr && curr->key == key)
		goto found;

not_found:
//	printf("not found: %ld\n", key);
#endif

#ifdef DEBUG
	printf("pskip_find_level <== end \n");
#endif
	return 0;

} 

int skip_list_put (long key, long data, struct skip_list_t* sl) {
	
	struct node_t *curr, *prev, *nnode;
	struct node_t **back_ptr[MAX_LEVEL];
	struct node_t *exp_ptr[MAX_LEVEL];
	long exp_data;

	int nlevel; 
	int i, cnt;

	if (sl == NULL || sl->list_head == NULL ){
		printf("skip list has null pointer");
		return -1;
	}

retry:

	pskip_find_level(key, sl, back_ptr, exp_ptr, 0);

	// insert new  
	nlevel = get_level();
	//printf("get_level = %d\n", nlevel);
	level_count[nlevel-1]++;

	if((nnode = (struct node_t*) malloc (sizeof(struct node_t))) == NULL) {
		printf("Failed to malloc for nnode\n");
		return -1;
	}

	nnode->key = key;
	nnode->data = data;

	for (i=0; i<nlevel; i++)
		nnode->next[i] = exp_ptr[i];

	// if level[0] update is failed, search again  
	if((__sync_val_compare_and_swap(back_ptr[0], exp_ptr[0], nnode)) != exp_ptr[0]){
		//printf("failed to update level 0: *back_ptr[0] = %p, exp_ptr[0] = %p nnode = %p\n", *back_ptr[0], exp_ptr[0], nnode);
		free(nnode);
		goto retry;
	}

	// remaining ptr update 
	for (i=1; i<nlevel; i++){

		while(1) {	
		// CAS
			if((__sync_val_compare_and_swap (back_ptr[i], exp_ptr[i], nnode)) == exp_ptr[i])
				break; // success 
			//find_ptr_for_level(i);
			fail_cnt++;
			pskip_find_level(key, sl, back_ptr, exp_ptr, i);
		}
		//prev->next[i] = nnode;
	}

	// failed to update intermediate pointers
	if ( i < nlevel){
		nnode->level = i; // not critical section 		
	}

	sl->node_cnt++; 

	return 0;

#if 0
found:
	// CAS 
	do {
		exp_data = curr->data;
	}while((__sync_val_compare_and_swap(&curr->data, exp_data, data)) != exp_data);
#endif
//	curr->data = data; 
	return 0;

}

//void print_skip_list_stat (struct skip_list_t* sl){
void skip_list_print_all (struct skip_list_t* sl){
	
	printf("fail_cnt: %d\n", fail_cnt);
	
	//for(int i = 0; i < MAX_LEVEL; i++)
//		printf("%d: %d\n", i+1, level_count[i]);

#if 0 
	int i;
	struct node_t* curr;

	// print skip list 
	for (i=MAX_LEVEL-1; i>=0; i--){
		curr = sl->list_head;
		while(curr) {
			printf("%ld:%ld --> ", curr->key, curr->data);
			curr = curr->next[i];
		}
		printf("\n");
	}	
#endif
}


