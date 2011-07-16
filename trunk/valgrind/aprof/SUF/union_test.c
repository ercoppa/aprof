#include "union_find.h"

int main(void) {
	UnionFind * u = UF_create();
	
	UF_insert(u, 4000, 1);
	UF_insert(u, 4004, 1);
	UF_insert(u, 4008, 1);
	UF_print(u);
	UF_insert(u, 4012, 1);
	UF_print(u);
	UF_insert(u, 4016, 2);
	UF_insert(u, 4004, 2);
	UF_insert(u, 4012, 2);
	UF_insert(u, 4020, 2);
	UF_print(u);
	UF_merge(u, 2);
	UF_print(u);
	UF_insert(u, 4024, 2);
	UF_insert(u, 4028, 2);
	UF_insert(u, 4016, 2);
	UF_insert(u, 4032, 2);
	UF_print(u);
	UF_lookup(u, 4004);
	UF_print(u);
	UF_rebalance(u, u->headRep->tree);
	UF_print(u);
	UF_rebalance(u, u->headRep->next->tree);
	UF_print(u);
	UF_merge(u, 2);
	UF_print(u);
	UF_rebalance(u, u->headRep->tree);
	UF_print(u);
	
	UF_destroy(u);
	return 0;
}
