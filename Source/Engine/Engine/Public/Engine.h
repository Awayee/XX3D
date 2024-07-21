#pragma once
namespace Engine {
	// singleton
	class XXEngine {
	public:
		XXEngine();
		virtual ~XXEngine();
		void Run();
		static void ShutDown();
	protected:
		bool m_Running;
		virtual void Update();
	};
}

