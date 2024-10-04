#include "Core/Public/Container.h"



FreeListAllocator::FreeListAllocator(FreeListAllocator&& rhs) noexcept : m_Ranges(MoveTemp(rhs.m_Ranges)) {}

FreeListAllocator& FreeListAllocator::operator=(FreeListAllocator&& rhs) noexcept {
	m_Ranges = MoveTemp(rhs.m_Ranges);
	return *this;
}

uint32 FreeListAllocator::Allocate(uint32 allocSize) {
	auto* node = m_Ranges.Head();
	if (!node) {
		return INVALID;
	}
	Range& range = node->Value;
	if (range.End - range.Start < allocSize) {
		return INVALID;
	}
	const uint32 result = range.Start;
	range.Start += allocSize;
	if (range.Start >= range.End) {
		m_Ranges.RemoveNode(m_Ranges.Head());
	}
	return result;
}

void FreeListAllocator::Free(uint32 allocStart, uint32 allocSize) {
	bool bFound = false;
	for (auto* node = m_Ranges.Head(); node; node = node->Next) {
		Range& range = node->Value;
		CHECK(range.Start < range.End);
		// check if equal to start or end
		if (range.Start == (allocStart + allocSize)) {
			range.Start = allocStart;
			bFound = true;
		}
		else if (range.End == allocStart) {
			range.End += allocSize;
			bFound = true;
		}
		else {
			// check if less than range
			CHECK(range.End < allocStart || range.Start > allocStart);
			if (range.Start > allocStart) {
				m_Ranges.InsertBefore({ allocStart, allocStart + allocSize }, node);
				bFound = true;
			}
		}
		if (bFound) {
			break;
		}
	}
	// otherwise insert at the right
	if (!bFound) {
		m_Ranges.InsertAfterTail({ allocStart, allocStart + allocSize });
	}
}

bool FreeListAllocator::IsEmpty() const {
	return m_Ranges.Size() == 0;
}