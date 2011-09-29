int foo (int x, int y) {
	return x+y;
}

int main() {
	
	int i = 0, r = 0;
	while(i++ < 1000)
		r = foo(i, i*2);
		
	return 0;
}
