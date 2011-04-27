
int fun(int a) {
	int i, res;
	res = 0;
	for (i = 0; i < a; i++) {
		res = res + i;
	}
	return res;
}

int main() {
	fun(100);
}
