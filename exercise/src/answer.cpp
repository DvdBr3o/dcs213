#include "function.h"

Node* reverse(Node* n) {
	if (!n->next)
		return n;

	Node* f = n;
	Node* s = n;

	f->next = nullptr;

	while (s) {
		const auto ss = s->next;
		s->next		  = f;
		f			  = s;
		s			  = ss;
	}

	return f;
}

Node* ReverseMergeList(Node* list1, Node* list2) {
	Node* res = nullptr;
	while (list1 && list2) {
		Node* newnode;
		if (list1->value < list2->value) {
			newnode = new Node { list1->value };
			list1	= list1->next;
		} else {
			newnode = new Node { list2->value };
			list2	= list2->next;
		}
		newnode->next = res;
		res			  = newnode;
	}
	while (list1) {
		auto* newnode = new Node { list1->value };
		list1		  = list1->next;
		newnode->next = res;
		res			  = newnode;
	}
	while (list2) {
		auto* newnode = new Node { list2->value };
		list2		  = list2->next;
		newnode->next = res;
		res			  = newnode;
	}

	return res;
}