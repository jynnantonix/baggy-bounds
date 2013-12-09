#include <cstdio>

extern "C" {
	void baggy_init();
}

static int initialization() {
	baggy_init();
	return 1;
}

int baggy_force_initialization = initialization();
