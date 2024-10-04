#include "Objects/Public/ECS.h"

namespace Object {

	ComponentID RegisterComponent() {
		static ComponentID s_ComponentMax{ 0 };
		ASSERT(s_ComponentMax < NUM_COMPONENT_MAX, "Component id out of range!");
		return s_ComponentMax++;
	}
}
