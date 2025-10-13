#pragma once

struct Node {
	Node* next;
	int	  value;

	Node(int val) : value(val), next(nullptr) {}
};

Node* getSum(Node* a, Node* b);