#pragma once
#include "AssetTree.h"

namespace Editor {
	class AssetsMgr {
	private:
		AssetNode* m_Root {nullptr};
		void BuildAssetsTree();
	public:
		AssetsMgr();
	};
}