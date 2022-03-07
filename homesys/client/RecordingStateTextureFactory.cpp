#include "stdafx.h"
#include "RecordingStateTextureFactory.h"

namespace DXUI
{
	RecordingStateTextureFactory::RecordingStateTextureFactory()
	{
	}

	bool RecordingStateTextureFactory::GetId(Homesys::RecordingState::Type state, std::wstring& id)
	{
		id.clear();

		switch(state)
		{
		case Homesys::RecordingState::Scheduled: id = L"state_scheduled.png"; break;
		case Homesys::RecordingState::InProgress: id = L"state_inprogress.png"; break;
		case Homesys::RecordingState::Aborted: id = L"state_aborted.png"; break;
		case Homesys::RecordingState::Missed: id = L"state_missed.png"; break;
		case Homesys::RecordingState::Error: id = L"state_error.png"; break;
		case Homesys::RecordingState::Finished: id = L"state_finished.png"; break;
		case Homesys::RecordingState::Overlapping: id = L"state_overlapping.png"; break;
		case Homesys::RecordingState::Prompt: id = L"state_recording.png"; break;
		case Homesys::RecordingState::Deleted: id = L"state_deleted.png"; break;
		case Homesys::RecordingState::Warning: id = L"state_warning.png"; break;
		}

		return !id.empty();
	}

	Texture* RecordingStateTextureFactory::GetTexture(Homesys::RecordingState::Type state, DeviceContext* dc)
	{
		std::wstring id;

		if(!GetId(state, id))
		{
			return NULL;
		}

		return dc->GetTexture(id.c_str());
	}

	Texture* RecordingStateTextureFactory::GetTexture(const Homesys::Program* program, DeviceContext* dc)
	{
		Texture* t = GetTexture(program->state, dc);

		if(t == NULL)
		{
			if(program->episode.movie.favorite)
			{
				t = dc->GetTexture(L"star_48.png");
			}
		}

		return t;
	}
}