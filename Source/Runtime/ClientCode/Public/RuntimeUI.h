#pragma once
#include "Core/Public/Defines.h"

namespace Runtime {
	class RuntimeUIMgr {
		SINGLETON_INSTANCE(RuntimeUIMgr);
	public:
		static void InitializeImGuiConfig();
		void Tick();
		bool NeedDraw();
	private:
		bool m_NeedDraw;
		RuntimeUIMgr();
		~RuntimeUIMgr() = default;
		void UpdateUI();
	};
}