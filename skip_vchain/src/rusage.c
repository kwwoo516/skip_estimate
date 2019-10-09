#include <stdio.h> 
#include <string.h> 
#include "rusage.h" 
#define ALPAH "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" 
#define NUMERIC "0123456789" 

static char* strstran(char* s) { 
	return strpbrk(s, ALPAH NUMERIC); 
}

 EntitySet rusage_create(pid_t pid) { 
	EntitySet usage; 
	usage.entities = calloc(60, sizeof(Entity)); 
	usage.capacity = 60; 
	usage.size = 0; 

	char path[100] = {0,}; 
	if(pid == 0) strcat(path, "/proc/self/status"); 
	else snprintf(path, sizeof(path) - 1, "/proc/%d/status", abs(pid)); 

	FILE* status = fopen(path, "r"); 
	if(!status) { 
		perror("cannot open status file"); 
		return usage; 
	} 
	
	char buf[65535]; 
	while(fgets(buf, sizeof(buf) - 1, status) && usage.size < usage.capacity) { 
		buf[strlen(buf) - 1] = '\0'; 
		char* key = buf; 
		char* value = strchr(buf, ':') + 1; 
		*(value - 1) = '\0'; 
		value = strstran(value); 
		if(value) { 
			char* space = strpbrk(value, " \t"); 
			if(space)
				 *space = '\0'; 
		}
		
		 Entity* e = &usage.entities[usage.size++];
		 e->key = strdup(key); 
		 e->value = strdup(value ? value : ""); 
	} 
	fclose(status); 
	return usage; 
} 

void rusage_destroy(EntitySet es) {	
	 for(int i = 0; i < es.size; ++i) { 
		Entity* e = &es.entities[i]; free(e->key); 
		free(e->value); 
		memset(e, 0, sizeof(Entity)); 
	}
	free(es.entities); 
	memset(&es, 0, sizeof(es)); 
}

char* rusage_getstr(EntitySet* es, char* key, char* def) { 
	for(int i = 0; i < es->size; ++i) { 
		Entity* e = &es->entities[i]; 
		if(strcmp(key, e->key) == 0) 
			return e->value[0] ? es->entities[i].value : def; 
	} 
	return def; 
}


