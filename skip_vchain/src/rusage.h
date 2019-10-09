#ifndef RUSAGE_RUSAGE_H 
#define RUSAGE_RUSAGE_H 
#include <stdlib.h>
 typedef struct {
 	char* key; 
	char* value; 
} Entity; 

typedef struct { 
	Entity* entities; 
	size_t size;
	size_t capacity; 
} EntitySet; 

EntitySet rusage_create(pid_t pid); 

void rusage_destroy(EntitySet es);

 char* rusage_getstr(EntitySet* es, char* key, char* def); 

#endif //RUSAGE_RUSAGE_H


