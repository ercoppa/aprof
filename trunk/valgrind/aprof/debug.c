#include "aprof.h"

#if DEBUG_ALLOCATION

static UInt alloc_type_size[A_NONE] = {
	sizeof(BB), sizeof(RoutineInfo), sizeof(Function), 
	sizeof(ThreadData), NAME_SIZE, sizeof(Activation),
	NAME_SIZE, 0, sizeof(HashNode),
	sizeof(SSM),  sizeof(SMSInfo),  sizeof(HashTable), sizeof(void *),
	sizeof(CCTS), sizeof(Object), NAME_SIZE
};

static char * alloc_type_name[A_NONE] = {
	"BasicBlock", "Routine", "Funzione", 
	"Thread", "FunctionName", "Activation",
	"ObjectName", "PoolPage", "HashNode",
	"SUF2Segment",  "SMSInfo",  "HashTable", "HT chain",
	"CCT", "Object", "Mangled"
};


static UInt alloc_counter[A_NONE] = { 0 };

void add_alloc(UWord type) {
	
	alloc_counter[type]++;
	/*
	if (alloc_counter[type] % 1000 == 0) {
		VG_(printf)("Allocated %u %s\n", 
			alloc_counter[type], alloc_type_name[type]); 
	}
	*/
	
	return;
	
}

void remove_alloc(UWord type) {
	
	alloc_counter[type]--;
	return;
	
}

void print_alloc(void) {
	
	UInt est = 0;
	
	VG_(printf)("Report allocations of aprof:\n\n");
	
	int i = 0;
	for (i = 0; i < A_NONE; i++) {
		VG_(printf)("Allocated %u %s ~ %u kb (%u mb)\n", 
			alloc_counter[i], alloc_type_name[i],
			(alloc_counter[i] * alloc_type_size[i]) / 1024,
			(alloc_counter[i] * alloc_type_size[i]) / 1024 / 1024);
		est += alloc_counter[i] * alloc_type_size[i];
	}
	
	VG_(printf)("\n");
	VG_(printf)("Estimated space usage: %u kb (%u mb)\n\n", est/1024, est/1024/1024);
}

#endif
