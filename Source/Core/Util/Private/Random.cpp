#include "Util/Public/Random.h"
#include <random>

namespace Util {

	float RandomF(float min, float max) {
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> distribution(min, max);
		return distribution(gen);
	}

	// reference https://en.wikipedia.org/wiki/Xorshift
	inline uint32 XorShift32(uint32 state) {
		state ^= state << 13;
		state ^= state >> 17;
		state ^= state << 5;
		return state;
	}

	uint32 RandomU32() {
		static uint32 s_State{ 123456789 };
		s_State = XorShift32(s_State);
		return s_State;
	}
}
