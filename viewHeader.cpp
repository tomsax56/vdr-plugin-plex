#include "viewHeader.h"

cViewHeader::cViewHeader(skindesignerapi::cViewElement* viewElem)
{
	m_pViewElem = std::shared_ptr<skindesignerapi::cViewElement>(viewElem);
	m_eCurrentTab = ePlexMenuTab::pmtOnDeck;
}

cViewHeader::~cViewHeader()
{
}

void cViewHeader::Draw(cGridElement* elem)
{
	m_pViewElem->Clear();
	m_pViewElem->ClearTokens();
	
	elem->AddTokens(m_pViewElem, false);
	
	switch(m_eCurrentTab) {
	case ePlexMenuTab::pmtOnDeck:
		m_pViewElem->AddIntToken("istab1active", 1);
		m_pViewElem->AddIntToken("istab2active", 0);
		m_pViewElem->AddIntToken("istab3active", 0);
		m_pViewElem->AddStringToken("tab1name", tr("On Deck"));
		break;
	case ePlexMenuTab::pmtRecentlyAdded:
		m_pViewElem->AddIntToken("istab1active", 0);
		m_pViewElem->AddIntToken("istab2active", 1);
		m_pViewElem->AddIntToken("istab3active", 0);
		m_pViewElem->AddStringToken("tab2name", tr("Recently Added"));
		break;
	case ePlexMenuTab::pmtLibrary:
		m_pViewElem->AddIntToken("istab1active", 0);
		m_pViewElem->AddIntToken("istab2active", 0);
		m_pViewElem->AddIntToken("istab3active", 1);
		m_pViewElem->AddStringToken("tab3name", tr("Library"));
		break;
	}

	m_pViewElem->Display();
}

ePlexMenuTab cViewHeader::NextTab()
{
	switch(m_eCurrentTab) {
	case ePlexMenuTab::pmtOnDeck:
		m_eCurrentTab = ePlexMenuTab::pmtRecentlyAdded;
		break;
	case ePlexMenuTab::pmtRecentlyAdded:
		m_eCurrentTab = ePlexMenuTab::pmtLibrary;
		break;
	case ePlexMenuTab::pmtLibrary:
		m_eCurrentTab = ePlexMenuTab::pmtOnDeck;
		break;
	}
	return m_eCurrentTab;
}

ePlexMenuTab cViewHeader::PrevTab()
{
	switch(m_eCurrentTab) {
	case ePlexMenuTab::pmtOnDeck:
		m_eCurrentTab = ePlexMenuTab::pmtLibrary;
		break;
	case ePlexMenuTab::pmtRecentlyAdded:
		m_eCurrentTab = ePlexMenuTab::pmtOnDeck;
		break;
	case ePlexMenuTab::pmtLibrary:
		m_eCurrentTab = ePlexMenuTab::pmtRecentlyAdded;
		break;
	}
	return m_eCurrentTab;
}
