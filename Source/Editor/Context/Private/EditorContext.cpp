#include "Context/Public/EditorContext.h"

namespace Editor {
	EditorContext* Context() {
		static EditorContext s_Context;
		return &s_Context;
	}

}
