#include "skip.h"

int level_count[12];

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
			else if (key < curr->key) 
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

int skip_list_put (long key, long data, struct skip_list_t* sl) {
	
	struct node_t *curr, *prev, *nnode;
	struct node_t **back_ptr[MAX_LEVEL];
	struct node_t *exp_ptr[MAX_LEVEL];
	long exp_data;
	int level, nlevel; 
	int i, cnt;

	if (sl == NULL || sl->list_head == NULL ){
		printf("skip list has null pointer");
		return -1;
	}

retry:
	cnt = 0;
	// search position  
	level = MAX_LEVEL-1;
	prev = sl->list_head;

	while (level >= 0) { // search nodes in all levels
		curr = prev->next[level]; 
		while (curr) {
			if (key < curr->key) // should search to level=0 to keep track of back pointers 
				break;
			if (key == curr->key){
				break;
			} 
			prev = curr;//새로 들어 온 노드의 key가 가장 큰 경우 마지막에 배치
			curr = curr->next[level];  
			cnt++;
			assert (cnt <= sl->node_cnt);
		}
		// tracking back pointers 
		back_ptr[level] = &prev->next[level];//(새 노드의 주소를 가리키는 next)를 가리키는 포인터
		exp_ptr[level] = curr;//마지막노드를 가리키는 포인터
		
		// move to lower layar
		level--;
	}
	assert (level == -1);
#if 0
	if (curr && curr->key == key)
		goto found;

not_found:
#endif
//	printf("not found: %ld\n", key);

	nlevel = get_level();//random하게 level설정
	level_count[nlevel-1]++;//level 별 개수파악

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
		if((__sync_val_compare_and_swap (back_ptr[i], exp_ptr[i], nnode)) != exp_ptr[i])
			break;
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

//	curr->data = data; 
	return 0;
#endif

}
struct skip_iter* skip_list_seek(long key, struct skip_iter * temp_iter, struct skip_list_t * sl){
	struct node_t *curr, *prev, *nnode;
	int level, nlevel; 
	int i;

	if (sl == NULL){
		printf("skip list has null pointer");
		return NULL;
	}

	// search position  
	level = MAX_LEVEL-1;
	prev = sl->list_head;

	while (level >= 0) { // search nodes in all levels
		curr = prev->next[level]; 
		while (curr) {
			if (key == curr->key){ 
				temp_iter->key=key;
				temp_iter->data=curr->data;
				temp_iter->curr=curr;
				return temp_iter;
//				return curr->v_chain->data; //latest data return 
//				return curr->data;
			}
			if (key < curr->key){ 
				temp_iter->key=curr->key;
				temp_iter->data=curr->data;
				temp_iter->curr=curr;
				break;
			}
			prev = curr;
			curr = curr->next[level];  
		}
		// move to lower layar
		level--;
	}	
	assert (level == -1);
	assert (prev != NULL);
	return temp_iter;	

}
int skip_list_next(struct skip_iter * sk_iter){

	if(!sk_iter->curr->next[0]){
		//printf("current node is end. key : %ld \n", sk_iter->key);
		return 0;
	}

	long key = sk_iter->key;
	long next_key = sk_iter->curr->next[0]->key;
	long data = sk_iter->data;
	long next_data= sk_iter->curr->next[0]->data;

	struct node_t * prev_node = sk_iter->curr->next[0];
	struct node_t * next_node = sk_iter->curr->next[0];
	
	//printf("key : %ld, next_key : %ld\n", key, next_key);
	while(key == next_key){
		next_node = prev_node->next[0];
		if(!next_node){
			//printf("current node is end. key : %ld \n", sk_iter->key);
			return 0;
		}
		prev_node = next_node;
		next_key  = next_node->key;	
		next_data = next_node->data;
		sk_iter->skip_cnt++;
	}
	sk_iter->data = next_node->data;
	sk_iter->key  = next_node->key;
	sk_iter->curr = next_node->next[0];
	return 1;
}
int skip_list_iter_valid(struct skip_iter * sk_iter){
	//if(sk_iter->key == sk_iter->curr->next[0]->key && sk_iter->data == sk_iter->curr->next[0]->data)
	if(!sk_iter->curr)
		return 0; //invalid //next_node is end node
	else
		return 1; //valid
}
int skip_list_range_query(long key, long next_count, struct skip_list_t* sl)
{
	long itercount=0;
	struct skip_iter* sk_iter;
	int iter_skip_cnt=0;
	if((sk_iter = (struct skip_iter *) malloc (sizeof ( struct skip_iter ))) == NULL){
				//printf("Failed to malloc for skip_iter \n");
				return -1;
	}
	sk_iter->skip_cnt=0;
	for(skip_list_seek(key, sk_iter, sl); skip_list_iter_valid(sk_iter) && itercount < next_count ; skip_list_next(sk_iter)){
		itercount++;
	}
	iter_skip_cnt=sk_iter->skip_cnt;
	free(sk_iter);
	return iter_skip_cnt;
	//printf("sk_iter_skip_count : %ld \n", sk_iter->skip_cnt);
	//printf("itercount : %ld \n", itercount);

}
void skip_list_print_all (struct skip_list_t* sl){

#if 0 
	for(int i = 0; i < MAX_LEVEL; i++)
		printf("%d: %d\n", i+1, level_count[i]);

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


