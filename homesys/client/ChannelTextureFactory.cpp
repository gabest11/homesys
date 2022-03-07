#include "stdafx.h"
#include "ChannelTextureFactory.h"
#include "client.h"

namespace DXUI
{
	ChannelTextureFactory::ChannelTextureFactory()
	{
		Control::DC.ChangedEvent.Add(&ChannelTextureFactory::OnDeviceContextChanged, this);
		
		DeactivatedEvent.Add(&ChannelTextureFactory::OnDeactivated, this);
	}

	ChannelTextureFactory::~ChannelTextureFactory()
	{
		RemoveAll();
	}

	void ChannelTextureFactory::RemoveAll()
	{
		for(auto i = m_map.begin(); i != m_map.end(); i++)
		{
			delete i->second->t;
			delete i->second;
		}

		m_map.clear();
	}

	bool ChannelTextureFactory::OnDeviceContextChanged(Control* c, DeviceContext* dc)
	{
		RemoveAll();

		return true;
	}

	bool ChannelTextureFactory::OnDeactivated(Control* c, int dir)
	{
		RemoveAll();

		return true;
	}

	Texture* ChannelTextureFactory::GetTexture(int channelId, DeviceContext* dc, Homesys::Channel* c)
	{
		Item* item = NULL;

		auto i = m_map.find(channelId);

		if(i != m_map.end())
		{
			item = i->second;
		}
		else
		{
			item = new Item();

			item->t = NULL;

			if(channelId > 0)
			{
				if(g_env->svc.GetChannel(channelId, item->c))
				{
					Util::ImageDecompressor id;

					if(id.Open(item->c.logo))
					{
						item->t = dc->CreateTexture(id.m_pixels, id.m_width, id.m_height, id.m_pitch);
					}
				}
			}

			m_map[channelId] = item;
		}

		if(item != NULL)
		{
			if(c != NULL)
			{
				*c = item->c;
			}

			return item->t;
		}

		return NULL;
	}
}