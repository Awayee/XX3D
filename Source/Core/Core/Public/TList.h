#pragma once
#include "Core/Public/Defines.h"

template<class T>
class TDoubleLinkList {
public:
	NON_COPYABLE(TDoubleLinkList);
	struct Node {
		T Value;
		Node* Prev;
		Node* Next;
	};
	TDoubleLinkList() : m_Head(nullptr), m_Tail(nullptr), m_Size(0) {}
	~TDoubleLinkList() {
		Empty();
	}

	TDoubleLinkList(TDoubleLinkList&& rhs) noexcept: m_Head(rhs.m_Head), m_Tail(rhs.m_Tail), m_Size(rhs.m_Size) {
		rhs.m_Head = rhs.m_Tail = nullptr;
		rhs.m_Size = 0;
	}

	TDoubleLinkList& operator=(TDoubleLinkList&& rhs) noexcept {
		m_Head = rhs.m_Head;
		m_Tail = rhs.m_Tail;
		m_Size = rhs.m_Size;
		rhs.m_Head = rhs.m_Tail = nullptr;
		rhs.m_Size = 0;
		return *this;
	}

	void Empty() {
		while(m_Head != nullptr) {
			Node* node = m_Head->Next;
			delete m_Head;
			m_Head = node;
		}
		m_Head = m_Tail = nullptr;
		m_Size = 0;
	}

	uint32 Size() const { return m_Size; }

	const Node* Head() const { return m_Head; }

	Node* Head() { return m_Head; }

	const Node* Tail() const { return m_Tail; }

	Node* Tail() { return m_Tail; }

	bool InsertBefore(const T& val, Node* nextNode) {
		if(!nextNode) {
			return false;
		}
		if(nextNode == m_Head) {
			InsertBeforeHead(val);
			return true;
		}
		Node* node = new Node{ val };
		node->Prev = nextNode->Prev;
		node->Next = nextNode;
		nextNode->Prev->Next = node;
		nextNode->Prev = node;
		++m_Size;
		return true;
	}

	void InsertAfterTail(const T& val) {
		Node* node = new Node{ val };
		if(m_Tail) {
			m_Tail->Next = node;
			node->Prev = m_Tail;
			m_Tail = node;
		}
		else {
			m_Head = m_Tail = node;
		}
		++m_Size;
	}

	void InsertBeforeHead(const T& val) {
		Node* node = new Node{ val };
		if (m_Head != nullptr){
			node->Next = m_Head;
			m_Head->Prev = node;
			m_Head = node;
		}
		else{
			m_Head = m_Tail = node;
		}
		++m_Size;
	}

	void RemoveNode(Node* node, bool isDelete=true) {
		if(node) {
			if(m_Size == 1) {
				if(isDelete) {
					Empty();
				}
				else {
					node->Next = node->Prev = nullptr;
					m_Head = m_Tail = nullptr;
					m_Size = 0;
				}
				return;
			}

			if(node == m_Head) {
				m_Head = m_Head->Next;
				m_Head->Prev = nullptr;
			}
			else if(node == m_Tail) {
				m_Tail = m_Tail->Prev;
				m_Tail->Next = nullptr;
			}
			else {
				node->Next->Prev = node->Prev;
				node->Prev->Next = node->Next;
			}
			if(node){
				delete node;
			}
			else{
				node->Next = node->Prev = nullptr;
			}
			--m_Size;
		}
	}

private:
	Node* m_Head;
	Node* m_Tail;
	uint32 m_Size;
};

