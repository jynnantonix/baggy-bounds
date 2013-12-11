#include <regex.h>
#include <cstdio>

static regex_t svcurls[5];

int main() {
	char regexp[] = "^.*$";
	printf("%d\n", regcomp(&svcurls[2], regexp, REG_EXTENDED | REG_NOSUB));
}
