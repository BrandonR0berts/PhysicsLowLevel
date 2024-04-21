#include "GlobalTrackers.h"

#include <malloc.h>

// ------------------------------------------------------

namespace Memory
{
	BaseTracker* mGenericTracker = nullptr;

	void SetupGlobalTrackers()
	{
		mGenericTracker    = (BaseTracker*)malloc(sizeof(BaseTracker));

		if(mGenericTracker)
			(*mGenericTracker) = BaseTracker();
	}
}

// ------------------------------------------------------