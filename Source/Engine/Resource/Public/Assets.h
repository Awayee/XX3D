#pragma once
#include "AssetType.h"

namespace Engine {
	class Assets {
		// all path are relative
	private:
		static String s_EngineAssetPath;
		static String s_ProjectAssetPath;
	public:
		template<typename T> static T LoadAsset(const String& file);
		template<typename T> static T LoadEngineAsset(const String& file);

		static void GetFiles(const String& folder, TVector<String>& files);
	};
}
