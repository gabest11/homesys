#pragma once

namespace DXUI
{
	class RecordingStateTextureFactory : public Control
	{
	public:
		RecordingStateTextureFactory();

		bool GetId(Homesys::RecordingState::Type state, std::wstring& id);
		Texture* GetTexture(Homesys::RecordingState::Type state, DeviceContext* dc);
		Texture* GetTexture(const Homesys::Program* program, DeviceContext* dc);
	};
}
