#include "Engine/Renderer/ResourceView.hpp"

bool ResourceViewInfo::operator==(ResourceViewInfo const& otherView) const
{
	return otherView.m_viewType == m_viewType;
}

ResourceViewInfo::~ResourceViewInfo()
{
	/*if (m_srvDesc) {
		delete m_srvDesc;
		m_srvDesc = nullptr;
	}

	if (m_rtvDesc) {
		delete m_rtvDesc;
		m_rtvDesc = nullptr;
	}

	if (m_cbvDesc) {
		delete m_cbvDesc;
		m_cbvDesc = nullptr;
	}

	if (m_uavDesc) {
		delete m_uavDesc;
		m_uavDesc = nullptr;
	}

	if (m_dsvDesc) {
		delete m_dsvDesc;
		m_dsvDesc = nullptr;
	}*/
}

ResourceView::ResourceView(ResourceViewInfo const& viewInfo):
	m_viewInfo(viewInfo),
	m_source(m_viewInfo.m_source)
{

}

ResourceView::~ResourceView()
{

}

