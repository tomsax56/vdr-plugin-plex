#include "plexSdOsd.h"
#include "viewGridNavigator.h"
#include "pictureCache.h"
#include <vdr/thread.h>

cMutex cPlexSdOsd::RedrawMutex;

cPlexSdOsd::cPlexSdOsd()
{
}

void cPlexSdOsd::Show(void)
{
	bool skinDesignerAvailable = InitSkindesignerInterface("plex");
	if (!skinDesignerAvailable) {
		return;
	}

	m_pRootView = GetOsdView(eViews::viRootView);
	if (!m_pRootView) {
		esyslog("[plex]: used skindesigner skin does not support plex");
		return;
	}
	
	m_pBrowserGrid = std::shared_ptr<cBrowserGrid>(new cBrowserGrid(m_pRootView));
	m_pBrowserGrid->SwitchGrid(ePlexMenuTab::pmtOnDeck);
	Flush();
}

void cPlexSdOsd::Flush()
{
	m_pBrowserGrid->DrawGrid();
	m_pBrowserGrid->Flush();
}

eOSState cPlexSdOsd::ProcessKey(eKeys Key)
{
	eOSState state = eOSState::osContinue;
	switch (Key & ~k_Repeat) {
	case kUp:
		m_pBrowserGrid->NavigateUp();
		Flush();
		break;
	case kDown:
		m_pBrowserGrid->NavigateDown();
		Flush();
		break;
	case kLeft:
		m_pBrowserGrid->NavigateLeft();
		Flush();
		break;
	case kRight:
		m_pBrowserGrid->NavigateRight();
		Flush();
		break;
	case kOk:
		// Play movie or change dir
		state = m_pBrowserGrid->NavigateSelect();
		Flush();
		break;
	case kBack:
		//state =
		m_pBrowserGrid->NavigateBack();
		Flush();
		break;
	case kRed:
		// Prev Tab
		//SwitchGrid(m_pViewHeader->NextTab());
		Flush();
		break;
	case kGreen:
		// Next Tab
		//SwitchGrid(m_pViewHeader->PrevTab());
		Flush();
		break;
	default:
		break;
	}
	return state;
}
