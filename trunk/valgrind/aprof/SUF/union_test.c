#include "union_find.h"

int main(void) {
	UnionFind * u = UF_create();
	
	UF_insert(u, 5000, 1);
	UF_insert(u, 5001, 1);
	UF_insert(u, 5002, 1);
	UF_print(u);
	UF_insert(u, 5003, 1);
	UF_print(u);
	UF_insert(u, 5004, 2);
	UF_insert(u, 5001, 2);
	UF_insert(u, 5003, 2);
	UF_insert(u, 5007, 2);
	UF_print(u);
	UF_merge(u, 2);
	UF_print(u);
	UF_insert(u, 5008, 2);
	UF_insert(u, 5009, 2);
	UF_insert(u, 5004, 2);
	UF_insert(u, 5011, 2);
	UF_print(u);
	UF_lookup(u, 5001);
	UF_print(u);
	//UF_rebalance(u, u->headRep->tree);
	//UF_rebalance(u, u->headRep->next->tree);
	//UF_print(u);
	UF_merge(u, 2);
	UF_print(u);
	UF_rebalance(u, u->headRep->tree);
	UF_print(u);
	
	UF_destroy(u);
	return 0;
}
