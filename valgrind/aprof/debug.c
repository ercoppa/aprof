#include "aprof.h"

#if DEBUG_ALLOCATION

static UInt alloc_type_size[A_NONE] = {
	sizeof(BB), sizeof(RoutineInfo), sizeof(Function), 
	sizeof(ThreadData), FN_NAME_SIZE, sizeof(Activation),
	FN_NAME_SIZE, 1024*sizeof(HashNode), sizeof(HashNode),
	sizeof(SSM),  sizeof(SMSInfo),  sizeof(HashTable),
	sizeof(CCTS)
};

static char * alloc_type_name[A_NONE] = {
	"BasicBlock", "Routine", "Funzione", 
	"Thread", "FunctionName", "Activation",
	"ObjectName", "PoolPage", "HashNode",
	"SUF2Segment",  "SMSInfo",  "HashTable",
	"CCT"
};


static UInt alloc_counter[A_NONE] = { 0 };

void add_alloc(UWord type) {
	
	alloc_counter[type]++;
	if (alloc_counter[type] % 1000 == 0) {
		VG_(printf)("Allocated %lu %s\n", 
			alloc_counter[type], alloc_type_name[type]); 
	}
	
	return;
	
}

void print_alloc(void) {
	
	VG_(printf)("Report allocations of aprof:\n\n");
	
	int i = 0;
	for (i = 0; i < A_NONE; i++)
		VG_(printf)("Allocated %lu %s\n", 
			alloc_counter[i], alloc_type_name[i]);
	
	VG_(printf)("\n");
	
}

#endif
