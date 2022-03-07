#include "stdafx.h"
#include "RadioButton.h"

namespace DXUI
{
	RadioButton::RadioButton()
	{
		ClickedEvent.Add0(&RadioButton::OnClicked, this, true);
		Checked.ChangedEvent.Add(&RadioButton::OnChecked, this);
	}

	bool RadioButton::OnClicked(Control* c)
	{
		if(!Checked)
		{
			Checked = true;
		}

		 return true;
	}

	bool RadioButton::OnChecked(Control* c, bool checked)
	{
		if(checked)
		{
			std::list<Control*> controls;

			GetSiblings(controls);

			for(auto i = controls.begin(); i != controls.end(); i++)
			{
				RadioButton* rb = dynamic_cast<RadioButton*>(*i);

				if(rb != NULL && rb->Group == Group)
				{
					rb->Checked = false;
				}
			}
		}

		return true;
	}
}