#include "skip.h"

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
	(*sl)->max_height = 0; // actual max level
	(*sl)->list_head->key = -1;
	(*sl)->list_head->data = -1;
	//(*sl)->list_head->data = NULL;

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
//		if(curr->data) free(curr->data);

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

	// return key
	return -1;
}

#if 0
static int get_level() {
	int rv, range, level;
	
	range = pow(2, MAX_LEVEL);
	rv = rand() % range;

	level = log(rv) / log(2);

	return level;

}
#endif

int skip_list_put (long key, long data, struct skip_list_t* sl) {
	
	struct node_t *curr, *prev, *nnode;
	struct node_t* test_node;
	struct node_t **back_ptr[MAX_LEVEL];
	int level, nlevel; 
	int i, cnt;

	// estimate
	int num_nodeLevel[MAX_LEVEL]; // num of node each level
	long k_est[MAX_LEVEL]; // estimate value
	long k_first = sl->key_first;
	long k_last = sl->key_last;
	int divide_val = 1; // node num + divide value(1) = each separate section num
	int Lv;

	// create new 
	nlevel = get_level();

	// renew max_height
	if(nlevel > sl->max_height){ 
	    sl->max_height = nlevel;
	}

    if((nnode = (struct node_t*) malloc (sizeof(struct node_t))) == NULL) {
		printf("Failed to malloc for nnode\n");
		return 0;
	}
	nnode->key = key;
	nnode->data = data;

#if 1
	//first, last init
	if(sl->node_cnt == 0){
	    sl->key_first = key;
	    sl->key_last = key;
	}
	
	// renew first, last key
	if (sl->key_first > key) {
		sl->key_first = key;
	}
	if (sl->key_last < key) {
		sl->key_last = key;
	}
#endif

	if (sl == NULL){
		printf("skip list has null pointer");
		return -1;
	}
	
	// search position
	level = MAX_LEVEL-1;
	prev = sl->list_head;
	
	// set estimate
	for (Lv = 0; Lv <= sl->max_height; Lv++) {
		num_nodeLevel[Lv] = (sl->node_cnt / pow(2, Lv));
	} // put할 때마다 계산.. 비효율적

	for (Lv = 0; Lv <= sl->max_height; Lv++) {
		k_est[Lv] = (k_last - k_first) / (long)(num_nodeLevel[Lv] + divide_val);
	}

	// 처음에 노드가 쌓일 시점엔 원래의 동작을 하는것이 낫겠음
	if (sl->node_cnt > 3) {
	    #define SWITCH
	}

#ifndef SWITCH
	while (level >= 0) { // search nodes in all levels
		curr = prev->next[level];
		while (curr) {
 #if 0
			// update (overwrite)
			if (key == curr->key)
				curr->data = data;
 #endif

			if (key <= curr->key)
				//if (key <= curr->key) 
				break;
			prev = curr;
			curr = curr->next[level];
			/*cnt++;
			assert(cnt <= sl->node_cnt);*/
		}
		// tracking back pointers 
		back_ptr[level] = &prev->next[level];

		// move to lower layar
		level--;
	}
	assert(level == -1);
#else
	// search - estimated ver
	while (level >= 0) {
		curr = prev->next[level];
		while (curr) {
			if (key == curr->key) {
				curr->data = data;
			}
			if (key < k_est[level] + curr->key) {
				break;
			}
			/*if (key < curr->next[level]->key) {
				break;
			}*/
			prev = curr;
			curr = curr->next[level];
			/*cnt++;
			assert(cnt <= sl->node_cnt);*/
		}
		back_ptr[level] = &prev->next[level];
		level--;
	}
	assert(level == -1);
#endif

	// put nnode
	for (i=0; i<nlevel; i++)
		nnode->next[i] = *back_ptr[i];

	for (i=0; i<nlevel; i++)
		*(back_ptr[i]) = nnode; 
	//	prev->next[i] = nnode;

	sl->node_cnt++;
	return 1;
}

struct skip_iter* skip_list_seek(long key, struct skip_iter * temp_iter, struct skip_list_t* sl){
	struct node_t *curr, *prev, *nnode;
	int level, nlevel; 
	int i;
	int cnt = 0;

	if (sl == NULL){
		printf("skip list has null pointer");
		return NULL;
	}

	// search position  
	level = MAX_LEVEL-1;
	prev = sl->list_head;

#if 1
	int num_nodeLevel[MAX_LEVEL]; // num of node each level
	long k_est[MAX_LEVEL]; // estimate value
	long total_cnt = sl->node_cnt; // total node num
	long k_first = sl->key_first;
	long k_last = sl->key_last;
	int divide_val = 1; // node num + divide value(1) = each separate section num

	for(int i=0; i<sl->max_height; i++){
	    num_nodeLevel[i] = (total_cnt/pow(2,i));
	}
	
	for(int i=0; i<sl->max_height; i++){
	    k_est[i] = (k_last - k_first)/(long)(num_nodeLevel[i] + divide_val);
	}

	// search - estimated ver
	while (level >= 0 ) {
	    curr = prev->next[level];
	    while(curr){
		if (key == curr->key){
		    temp_iter->key = key;
		    temp_iter->data = curr->data;
		    temp_iter->curr = curr;
		    return temp_iter;
		}
		if (key < k_est[level] + curr->key){
		    break;
		}
		if (key < curr->next[level]->key){
		    break;
		}
		prev = curr;
		curr = curr->next[level];
		cnt++;
		assert (cnt <= sl->node_cnt);
	    }
	    level--;
	}

#endif
#if 0
	while (level >= 0) { // search nodes in all levels
		curr = prev->next[level]; 
		while (curr) {
			if (key == curr->key){
				temp_iter->key=key;
				temp_iter->data=curr->data;
				temp_iter->curr=curr;
				//printf("seekdonw found!\n");
				return temp_iter;
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
#endif
	//printf("not found key : %ld, %ld", key,curr->key);
	assert (level == -1);
	assert (prev != NULL);
	//printf("seekdone not found!\n");
	//return temp_iter;
	return temp_iter;
}
int skip_list_next(struct skip_iter * sk_iter){
	//printf("next start before valid check\n");
	if(sk_iter->curr->next[0]==NULL){
		//printf("current node is end. key : %ld \n", sk_iter->key);
		return 0;
	}
	//printf("next start \n");
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
	//printf("next done\n");
	return 1;

}
int skip_list_iter_valid(struct skip_iter * sk_iter){
	if(sk_iter->curr)
		return 1; //valid
	else
		return 0; //invalid
}
int skip_list_range_query(long key, long next_count, struct skip_list_t* sl)
{
	long itercount=0;
	struct skip_iter* sk_iter;
	int iter_skip_cnt=0;
	if((sk_iter = (struct skip_iter *) malloc (sizeof ( struct skip_iter ))) == NULL){
				printf("Failed to malloc for skip_iter \n");
				return -1;
	}
	sk_iter->skip_cnt=0;

	for(skip_list_seek(key, sk_iter, sl); skip_list_iter_valid(sk_iter) && itercount < next_count ; skip_list_next(sk_iter)){
		itercount++;
	}

/*
	skip_list_seek(key, sk_iter,sl);
	do{
		skip_list_next(sk_iter);
		printf("%d\n",sk_iter->key);
		printf("%d\n",sk_iter->data);
		printf("%p\n",sk_iter->curr);
		itercount++;
	}while(skip_list_iter_valid(sk_iter) && itercount < next_count);
*/		

	iter_skip_cnt=sk_iter->skip_cnt;
	free(sk_iter);
	return iter_skip_cnt;
	//printf("sk_iter_skip_count : %ld \n", sk_iter->skip_cnt);
	//printf("itercount : %ld \n", itercount);
}
void skip_list_print_all (struct skip_list_t* sl){
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


