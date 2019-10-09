#include "skip.h"
#include <pthread.h>
#include "rusage.h"
#include "sys/types.h"
//#include "sys/sysinfo.h"

int level_count[12];



///////////////////////////////
//							 //
//       Leeju Kim 	         //
//							 //
///////////////////////////////

pthread_t rc_thread; // memory reclamation thread 
pthread_cond_t rc_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t rc_mutex = PTHREAD_MUTEX_INITIALIZER ;//unused

#ifdef DEBUG 
int put_count=0;
#endif

//const int remained_vchain_num = 1;
const int vc_len = 1;

#if 0
struct sysinfo memInfo;
long long totalVirtualMem;
#endif

void do_reclaim (struct skip_list_t* sl) {
retest:
{

	struct node_t* cursor;
	struct vcnode_t* free_list; // list of reclaimed nodes 


	// initialize 
	atomic_long chain_cnt = 0;





	// wake up and search 
	cursor = sl->list_head;

#if 0
	if ((head_remove_vcnode = (struct vcnode_t*) malloc(sizeof(struct vcnode_t))) == NULL) {
		//vchain node alloc fail
		printf("Failed to malloc for vcnode_t\n");
		return ;
	}

	head_remove_vcnode->next = NULL;
	tail_remove_vcnode = head_remove_vcnode;

#endif

	while (cursor != NULL) {
	retry:
		if (cursor->vchain_num > remained_vchain_num) {
			remain_vcnode = cursor->vchain;
			//남아 있을 노드들의 꼬리를 찾음
			for (long i = 1; i < remained_vchain_num; i++) {
				remain_vcnode = remain_vcnode->next;
			}
			//삭제할 노드들의 꼬리를 찾음
			while (tail_remove_vcnode->next != NULL) {
				tail_remove_vcnode = tail_remove_vcnode->next;
			}

//			if((__sync_val_compare_and_swap(&tail_remove_vcnode, NULL, remain_vcnode->next)) != NULL)
			*tail_remove_vcnode=remain_vcnode->next;
//			goto retry;
			remain_vcnode->next = NULL;
			cursor->vchain_num = remained_vchain_num;
			
		}
		chain_cnt += cursor->vchain_num;//원자적으로 실행
		cursor = cursor->next[0];
	}

	while (head_remove_vcnode != NULL) {
		temp_vcnode = head_remove_vcnode->next;
		free(head_remove_vcnode);
		head_remove_vcnode = temp_vcnode;
	}
	sl->chain_cnt = chain_cnt;
	pthread_cond_wait(&cond,&mutex);
	goto retest;
}
}

int skip_list_init(struct skip_list_t** sl) {
	
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
	(*sl)->list_head->vchain=NULL;
	(*sl)->chain_cnt= 0;
	//*((*sl)->list_head->data) = -1;
	//(*sl)->list_head->data = NULL;
	//pthread_mutex_init(&((*sl)->list_lock), NULL);
	int i;
	for ( i=0; i < MAX_LEVEL; i++)
		(*sl)->list_head->next[i] = NULL;

	// initialize level_prob_table 
	for (i=0; i<MAX_LEVEL; i++){
		prob_tbl[i] = pow(2, MAX_LEVEL-1-i);//??prob_tbl
	}
	for (i=MAX_LEVEL-2; i>=0; i--){
		prob_tbl[i] = prob_tbl[i] + prob_tbl[i+1];
	}

	// init random function for level decision
	srand((unsigned) time (NULL));

	for(i=0; i < MAX_LEVEL; i++)
		level_count[i] = 0;


	// Create a background memory reclamation thread 
	
	if(pthread_create(&remove_thread, NULL,(void *) do_reclaim, *sl) < 0)
	{
		perror("thread create error:");
		exit(0);	
	}
//	usage = rusage_create(0);	
	return 0;
}


void skip_list_destroy(struct skip_list_t* sl) {

	struct node_t* curr;
	struct node_t* next;
	struct vcnode_t* vc_curr;
	struct vcnode_t* vc_next;	
	int i=0;
	int j=0;

	if(sl == NULL)
		return;

	curr = sl->list_head; 
	// free all nodes 
	while (curr) {
		next = curr->next[0]; // free node linked in lowest-level
		vc_curr=curr->vchain;
		assert(vc_curr==curr->vchain);
#if 0
		while(vc_curr){
			if(vc_curr->next != NULL){
				vc_next=vc_curr->next;
				free(vc_curr);
				j++;
				vc_curr=NULL;
				vc_curr=vc_next;
				assert(vc_curr!=NULL);
			}else{
				free(vc_curr);
				vc_curr=NULL;
				j++;
			}
		}
#endif	
		free(curr);
		curr=NULL;
		vc_curr=NULL;
		curr = next;
		i++;
	}
	sl->list_head=NULL;
	//printf("free skip_list node_cnt %ld, chain_cnt %ld, node_free_num : %d, chain_free_num : %d\n",sl->node_cnt, sl->chain_cnt, i, j);
	//free(sl);
	pthread_cancel(remove_thread);
	sysinfo (&memInfo);
         totalVirtualMem = memInfo.totalram;
         totalVirtualMem += memInfo.totalswap;
         totalVirtualMem *= memInfo.mem_unit;
         printf("totalVirtualMem = %f\n",totalVirtualMem);
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
				return curr->vchain->data; 
			else if(key < curr->key) 
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
#if 0
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

	// search position  
	level = MAX_LEVEL-1;
	prev = sl->list_head;

	while (level >= low_level) { // search nodes in all levels
		curr = prev->next[level]; 
		while (curr) {
			if (key < curr->key) 
				break;
			else if (key == curr->key){
				/* do something */
				break;
			}
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
#endif


int skip_list_put (long key, long data, struct skip_list_t* sl) {
	
	struct node_t *curr, *prev, *nnode;
	struct node_t **back_ptr[MAX_LEVEL];
	struct node_t *exp_ptr[MAX_LEVEL];
	long exp_data;
	int exist_node=FALSE;
	struct vcnode_t* tmp_vchain_ptr;
	struct vcnode_t* new_vchain_ptr;
	int level, nlevel; 
	int i, cnt;




	if (sl == NULL || sl->list_head == NULL ){
		printf("skip list has null pointer");
		return -1;
	}

retry:
#if 1
	if((new_vchain_ptr = (struct vcnode_t*) malloc ( sizeof(struct vcnode_t))) == NULL){
			//vchain node alloc fail
			printf("Failed to malloc for vcnode_t\n");
			return -1;
	}
#endif 
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
				exist_node=TRUE;
				break;
			} 
			prev = curr;
			curr = curr->next[level];  
			cnt++;
			assert (cnt <= sl->node_cnt);
		}
		// tracking back pointers 
		back_ptr[level] = &prev->next[level];
		exp_ptr[level] = curr;
		
		// move to lower layar
		level--;
	}
	assert (level == -1);
	if(exist_node==FALSE){//key를 가진 노드가 없어서 노드를 새로 만든다
		nlevel = get_level();//level 랜덤 생성
		level_count[nlevel-1]++;
	
		if((nnode = (struct node_t*) malloc (sizeof(struct node_t))) == NULL) {
			printf("Failed to malloc for nnode\n");
			return -1;
		}
#if 1
		new_vchain_ptr->data=data;	
		//new_vchain_ptr->next=NULL;
		//new_vchain_ptr->t_stamp=1;
#endif
		nnode->key = key;
		nnode->vchain=new_vchain_ptr;
		nnode->vchain_num=1;

		for (i=0; i<nlevel; i++)
			nnode->next[i] = exp_ptr[i];//다음 노드랑 연결

		// if level[0] update is failed, search again  
		if((__sync_val_compare_and_swap(back_ptr[0], exp_ptr[0], nnode)) != exp_ptr[0]){//성공 return : exp_ptr[0], 실패 return : back_ptr[0] 
			//printf("failed to update level 0: *back_ptr[0] = %p, exp_ptr[0] = %p nnode = %p\n", *back_ptr[0], exp_ptr[0], nnode);			
			free(new_vchain_ptr);
			free(nnode);
			//new_vchain_ptr=NULL;
			//nnode=NULL;	
			goto retry;
		}
	
		// remaining ptr update 
		for (i=1; i<nlevel; i++){
			if((__sync_val_compare_and_swap (back_ptr[i], exp_ptr[i], nnode)) 
				!= exp_ptr[i]){
				//assert(1);

				free(new_vchain_ptr);
				free(nnode);
				goto retry;
			//	break;
			}
			//prev->next[i] = nnode;
		}

		// failed to update intermediate pointers
		if ( i < nlevel){//new node의 레벨 체크
			nnode->level = i; // not critical section 		
		}
		//assert(nnode->vchain);
		sl->node_cnt++;//node 갯수 증가
		sl->chain_cnt++;
//		printf("VmSize = %d\n",rusage_getstr(&usage,size,"nil"));
		if (sl->chain_cnt > (sl->node_cnt * remained_vchain_num)){
//		if(rusage_getstr(&usage,size,"nil") > 200000){
			pthread_cond_signal(&cond);
		}

#ifdef DEBUG
		 printf("put_count = %d\n",++put_count);
                 printf("node_cnt = %d\n",sl->node_cnt);
		printf("chain = %d\n\n",sl->chain_cnt);
#endif
		return 0;
	}else{
retry_vcnode:	
		assert(curr->vchain);
		new_vchain_ptr->data=data;
		new_vchain_ptr->next=curr->vchain;	
		//new_vchain_ptr->t_stamp=curr->vchain->t_stamp+1;
		if((__sync_val_compare_and_swap(&curr->vchain, new_vchain_ptr->next, new_vchain_ptr)) != new_vchain_ptr->next){
			free(new_vchain_ptr);
			new_vchain_ptr=NULL;
			goto retry;
		}
		curr->vchain_num++;
		assert(curr->vchain->next);
		sl->chain_cnt++;//전체 chain의 합
		
//		 printf("VmSize = %d\n",rusage_getstr(&usage,size,"nil")); 
		if (sl->chain_cnt > (sl->node_cnt * remained_vchain_num)){
//		if(rusage_getstr(&usage,size,"nil") > 20000000){
			pthread_cond_signal(&cond);
		}

		 printf("put_count = %d\n",++put_count);
                 printf("node_cnt = %d\n",sl->node_cnt);
                printf("chain = %d\n\n",sl->chain_cnt);
	}
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
struct skip_iter* skip_list_seek(long key, struct skip_iter* temp_iter ,struct skip_list_t* sl){
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
				temp_iter->data=curr->vchain->data;
				temp_iter->curr=curr;
				return temp_iter;
//				return curr->vchain->data; //latest data return 
//				return curr->data;
			}
			if (key < curr->key){ 
				temp_iter->key=curr->key;
				temp_iter->data=curr->vchain->data;
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

	sk_iter->curr=sk_iter->curr->next[0];
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
	if((sk_iter = (struct skip_iter *) malloc (sizeof ( struct skip_iter ))) == NULL){
				//printf("Failed to malloc for skip_iter \n");
				return -1;
	}
	for(skip_list_seek(key, sk_iter, sl); skip_list_iter_valid(sk_iter) && itercount < next_count ; skip_list_next(sk_iter)){
		itercount++;
	}
	//printf("itercount : %ld \n", itercount);
	free(sk_iter);	
	return 0;
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



