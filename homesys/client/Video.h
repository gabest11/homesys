#pragma once

namespace DXUI
{
	class Video : public Button
	{
		Animation m_anim;
		
		bool OnPaintForeground(Control* c, DeviceContext* dc);

	public:
		Video();

		Property<int> Selected;
		Property<bool> MirrorEffect;
		Property<bool> ZoomEffect;
	};

	class VideoNF : public Video
	{
	public:
		VideoNF() {CanFocus = 0;}
	};
}
