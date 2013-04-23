long long fun(long long a) {
	long long i, res;
	res = 0;
	for (i = 0; i < 100; i++) {
		res = res + i;
	}
	return res;
}

int main() {
    int res;
    //printf("program start\n");
    res = fun(100000);
    //printf("program end\n");
    return 0;
}
