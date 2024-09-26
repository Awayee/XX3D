#pragma once

namespace Editor {
	class UIController {
	private:
	public:
		static void InitializeImGuiConfig();
		UIController();
		~UIController();
		void Tick();
	};
}
