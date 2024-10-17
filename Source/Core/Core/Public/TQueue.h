#pragma once
#include <iostream>
#include "Core/Public/Defines.h"

// A thead unsafe queue
template<class T>
class TQueue {
public:
	TQueue(): m_Tail(nullptr), m_Head(nullptr), m_Size(0) {
		m_Head = m_Tail = nullptr;
	}
	~TQueue() {
		while(nullptr != m_Tail) {
			TNode* node = m_Tail;
			m_Tail = m_Tail->NextNode;
			delete node;
		}
	}
	bool IsEmpty() const {
		return nullptr == m_Tail;
	}

	// Add to head
	void Enqueue(const T& data) {
		TNode* newNode = new TNode(data);
		InnerEnqueue(newNode);
	}
	void Enqueue(T&& data) {
		TNode* newNode = new TNode(MoveTemp(data));
		InnerEnqueue(newNode);
	}
	// Get tail
	const T* Back() {
		if(nullptr == m_Tail) {
			return nullptr;
		}
		return &m_Tail->Data;
	}

	// Pop tail
	bool Pop() {
		if(nullptr == m_Tail) {
			return false;
		}
		TNode* oldTail = m_Tail;
		m_Tail = m_Tail->NextNode;
		// check if empty
		if(nullptr == m_Tail) {
			m_Head = nullptr;
		}
		delete oldTail;
		--m_Size;
		return true;
	}

	// empty
	void Reset() {
		while (Pop()) {}
	}

	uint32 Size() const {
		return m_Size;
	}

private:
	struct TNode {
		T Data;
		TNode* NextNode;
		TNode(): NextNode(nullptr) {}
		TNode(const T& data): Data(data), NextNode(nullptr) {}
		TNode(T&& data) : Data(MoveTemp(data)), NextNode(nullptr){}
	};
	// link order is m_Tail-> ... -> m_Head
	TNode* m_Head;
	TNode* m_Tail;
	uint32 m_Size;

	void InnerEnqueue(TNode* newNode) {
		if(nullptr == m_Head) {
			m_Head = m_Tail = newNode;
		}
		else {
			TNode* oldHead = m_Head;
			m_Head = newNode;
			oldHead->NextNode = newNode;
		}
		++m_Size;
	}
};