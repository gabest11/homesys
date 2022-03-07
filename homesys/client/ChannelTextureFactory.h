#pragma once

namespace DXUI
{
	class ChannelTextureFactory : public Control
	{
		bool OnDeviceContextChanged(Control* c, DeviceContext* dc);
		bool OnDeactivated(Control* c, int dir);

		struct Item {Texture* t; Homesys::Channel c;};

		std::map<int, Item*> m_map;

		void RemoveAll();

	public:
		ChannelTextureFactory();
		virtual ~ChannelTextureFactory();

		Texture* GetTexture(int channelId, DeviceContext* dc, Homesys::Channel* c = NULL);
	};
}
