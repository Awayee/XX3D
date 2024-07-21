#pragma once
#include "Core/Public/Defines.h"

namespace Math {

	// [a, b)
	template <typename T> struct Range {
		T A;
		T B;
		Range(T a, T b): A(a), B(b){}
	};

	template<typename T> class IterRange {
	private:
		class Iter {
		private:
			T m_Var;
			T m_Step;
		public:
			Iter(T var, T step): m_Var(var), m_Step(step){}

			T operator*() {
				return m_Var;
			}

			Iter& operator++() {
				m_Var += m_Step;
				return *this;
			}

			bool operator != (const Iter& rhs) {
				return m_Var != rhs.m_Var;
			}
		};
	public:
		IterRange(T begin, T end, T step): m_Begin(begin), m_End(end), m_Step(step){}

		Iter begin() {
			return Iter(m_Begin, m_Step);
		}

		Iter end() {
			return Iter(m_End, m_Step);
		}

	private:
		T m_Begin;
		T m_End;
		T m_Step;
	};

	typedef IterRange<int>     IterRangeI;
	typedef IterRange<uint32>  IterRangeU;
	typedef IterRange<uint64>  IterRangeUL;
	typedef IterRange<uint8>   IterRangeC;
}