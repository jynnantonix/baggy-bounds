/*
LANG: C++
*/

#include <cstdio>
#include <algorithm>
#include <cstdlib>
using namespace std;

#define NMAX 500005

#define FULL 0
#define EMPTY 1
#define NORMAL 2
struct Node {
	int a, b;
	int maxl, maxr, maxm;
	char status;
	Node* l;
	Node* r;
};

Node* init_tree(int a, int b) {
	Node* v = new Node();
	v->a = a;
	v->b = b;
	v->maxl = b - a;
	v->maxr = b - a;
	v->maxm = b - a;
	v->status = NORMAL;
	if (b - a > 1) {
		v->l = init_tree(a, (a+b)/2);
		v->r = init_tree((a+b)/2, b);
	} else {
		v->l = NULL;
		v->r = NULL;
	}
	return v;
}

inline void propogate(Node *v) {
	if (v->status == EMPTY) {
		v->l->maxl = v->l->b - v->l->a;
		v->l->maxr = v->l->b - v->l->a;
		v->l->maxm = v->l->b - v->l->a;
		v->l->status = EMPTY;
		v->r->maxl = v->r->b - v->r->a;
		v->r->maxr = v->r->b - v->r->a;
		v->r->maxm = v->r->b - v->r->a;
		v->r->status = EMPTY;
	}
	else if (v->status == FULL) {
		v->l->maxl = 0;
		v->l->maxr = 0;
		v->l->maxm = 0;
		v->l->status = FULL;
		v->r->maxl = 0;
		v->r->maxr = 0;
		v->r->maxm = 0;
		v->r->status = FULL;
	}
	v->status = NORMAL;
}

int get_location(Node* v, int req) {
	if (v->l == NULL) {
		if (req <= 1) {
			return v->a;
		} else {
			return -1;
		}
	}

	propogate(v);

	if (v->l->maxm >= req) {
		return get_location(v->l, req);
	}
	else if (v->l->maxr + v->r->maxl >= req) {
		return v->l->b - v->l->maxr;
	}
	else if (v->r->maxm >= req) {
		return get_location(v->r, req);
	}
	else {
		return -1;
	}
}

void set_range(Node* v, int a, int b, bool full) {
	if (a <= v->a && b >= v->b) {
		if (full) {
			v->maxl = 0;
			v->maxr = 0;
			v->maxm = 0;
			v->status = FULL;
		} else {
			v->maxl = v->b - v->a;
			v->maxr = v->b - v->a;
			v->maxm = v->b - v->a;
			v->status = EMPTY;
		}
		return;
	}

	if (v->l == NULL) {
		return;
	}

	propogate(v);

	if (a < v->l->b && b > v->l->a) {
		set_range(v->l, a, b, full);
	}
	if (a < v->r->b && b > v->r->a) {
		set_range(v->r, a, b, full);
	}

	v->maxl = (v->l->maxl == v->l->b - v->l->a ? v->l->maxl + v->r->maxl : v->l->maxl);
	v->maxr = (v->r->maxr == v->r->b - v->r->a ? v->r->maxr + v->l->maxr : v->r->maxr);
	v->maxm = max(max(v->l->maxm, v->r->maxm), v->l->maxr + v->r->maxl);
}

int main() {
	int n, m;
	scanf("%d", &n);
	scanf("%d", &m);

	Node* root = init_tree(0, n);
	int numTurnedAway = 0;

	for (int i = 0; i < m; i++) {
		int c;
		scanf("%d", &c);
		if (c == 0) {
			int p;
			scanf("%d", &p);
			int loc = get_location(root, p);
			if (loc == -1) {
				numTurnedAway++;
			} else {
				set_range(root, loc, loc + p, true);
			}
		}
		else {
			int a, b;
			scanf("%d", &a);
			scanf("%d", &b);
			set_range(root, a-1, b, false);
		}
	}

	printf("%d\n", numTurnedAway);
}
