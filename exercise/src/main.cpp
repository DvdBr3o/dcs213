#include <iostream>
#include "List.h"
using namespace std;

int main() {
	int n, m;
	cin >> n >> m;

	int	  tmp;
	Node *a = nullptr, *b = nullptr, *cur = nullptr;

	for (int i = 0; i < n; i++) {
		cin >> tmp;
		Node* node_ptr = new Node(tmp);
		if (a == nullptr) {
			a	= node_ptr;
			cur = node_ptr;
		} else {
			cur->next = node_ptr;
			cur		  = cur->next;
		}
	}

	for (int i = 0; i < m; i++) {
		cin >> tmp;
		Node* node_ptr = new Node(tmp);
		if (b == nullptr) {
			b	= node_ptr;
			cur = node_ptr;
		} else {
			cur->next = node_ptr;
			cur		  = cur->next;
		}
	}

	Node* c = getSum(a, b);

	while (c != nullptr) {
		cout << c->value << " ";
		cur = c;
		c	= c->next;
		delete cur;
	}

	while (a != nullptr) {
		cur = a;
		a	= a->next;
		delete cur;
	}

	while (b != nullptr) {
		cur = b;
		b	= b->next;
		delete cur;
	}

	return 0;
}