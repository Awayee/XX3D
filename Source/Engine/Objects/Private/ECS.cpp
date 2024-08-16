#include "Objects/Public/ECS.h"

namespace Object {

	static ComponentID s_ComponentMax{ 0 };
	ComponentID RegisterComponent() {
		ASSERT(s_ComponentMax < NUM_COMPONENT_MAX, "Component id out of range!");
		return s_ComponentMax++;
	}
}
