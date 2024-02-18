#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/NamedProperties.hpp"

EventSystem* g_theEventSystem = nullptr;

EventFuncSubscription::EventFuncSubscription(EventCallbackFunction callbackFunction) :
	m_callbackFunction(callbackFunction)
{

}

bool EventFuncSubscription::Execute(EventArgs& eventArgs) const
{
	return (m_callbackFunction)(eventArgs);
}

bool EventFuncSubscription::IsSameFunction(void* otherFunction) const
{
	EventCallbackFunction* otherFuncAsCallback = reinterpret_cast<EventCallbackFunction*>(otherFunction);
	return m_callbackFunction == *otherFuncAsCallback;
}

EventSystem::EventSystem(EventSystemConfig const& config) :
	m_config(config)
{
}

EventSystem::~EventSystem()
{

}

void EventSystem::Startup()
{
}

void EventSystem::Shutdown()
{
	for (auto it = m_subscriptionListByEventName.begin(); it != m_subscriptionListByEventName.end(); it++) {
		SubscriptionList& subList = it->second;
		for (int index = 0; index < subList.size(); index++) {
			EventSubscription*& eventSub = subList[index];
			delete eventSub;
		}
		subList.clear();
	}
}

void EventSystem::BeginFrame()
{
}

void EventSystem::EndFrame()
{
}

void EventSystem::GetRegisteredEventNames(std::vector< std::string >& outNames) const
{
	m_subsListMutex.lock();
	outNames.reserve(m_subscriptionListByEventName.size());
	for (std::map<std::string, SubscriptionList>::const_iterator iter = m_subscriptionListByEventName.begin(); iter != m_subscriptionListByEventName.end(); iter++) {
		SubscriptionList const& eventSubList = iter->second;
		if (eventSubList.size() > 0) {
			outNames.push_back(iter->first);
		}
	}

	m_subsListMutex.unlock();
}


void EventSystem::SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunction functionPtr)
{
	EventFuncSubscription* newSubscription = new EventFuncSubscription(functionPtr);

	m_subsListMutex.lock();

	std::map<std::string, SubscriptionList>::iterator iter = m_subscriptionListByEventName.find(eventName);
	if (iter == m_subscriptionListByEventName.end()) {
		SubscriptionList eventSubList;
		eventSubList.emplace_back(newSubscription);
		m_subscriptionListByEventName[eventName] = eventSubList;
	}
	else {
		SubscriptionList& eventSubList = iter->second;
		for (int subIndex = 0; subIndex < eventSubList.size(); subIndex++) {
			if (eventSubList[subIndex]->IsSameFunction(&functionPtr)) {
				ThrowError("THERE WAS AN ATTEMPT TO DOUBLE SUBSCRIBE A FUNCTION TO AN EVENT");
			}
		}
		eventSubList.emplace_back(newSubscription);
	}

	m_subsListMutex.unlock();
}

void EventSystem::UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunction functionPtr)
{
	m_subsListMutex.lock();

	std::map<std::string, SubscriptionList>::iterator iter = m_subscriptionListByEventName.find(eventName);
	if (iter == m_subscriptionListByEventName.end()) {
		m_subsListMutex.unlock();
		return;
	}
	else {
		SubscriptionList& eventSubList = iter->second;
		for (int subIndex = 0; subIndex < eventSubList.size(); subIndex++) {
			if (eventSubList[subIndex]->IsSameFunction(&functionPtr)) {
				delete eventSubList[subIndex];
				eventSubList.erase(eventSubList.begin() + subIndex);
				m_subsListMutex.unlock();
				return;
			}
		}
	}

	m_subsListMutex.unlock();
}

bool EventSystem::FireEvent(std::string const& eventName, EventArgs& args)
{
	SubscriptionList const* eventSubList = nullptr;

	m_subsListMutex.lock();

	for (std::map<std::string, SubscriptionList>::const_iterator iter = m_subscriptionListByEventName.begin(); iter != m_subscriptionListByEventName.end(); iter++) {
		if (!_stricmp(iter->first.c_str(), eventName.c_str())) { // Strings without case sensitivity. 0 == strings are equal
			eventSubList = &iter->second;
			break;
		}
	}

	m_subsListMutex.unlock();

	if (!eventSubList) {
		return false;
	}
	else {

		for (int subIndex = 0; subIndex < eventSubList->size(); subIndex++) {
				bool wasConsumed = eventSubList->at(subIndex)->Execute(args);
				if (wasConsumed) return true;
		}
	}

	return true;
}

bool EventSystem::FireEvent(std::string const& eventName)
{
	SubscriptionList const* eventSubList = nullptr;

	m_subsListMutex.lock();

	for (std::map<std::string, SubscriptionList>::const_iterator iter = m_subscriptionListByEventName.begin(); iter != m_subscriptionListByEventName.end(); iter++) {
		if (!_stricmp(iter->first.c_str(), eventName.c_str())) {
			eventSubList = &iter->second;
			break;
		}
	}

	m_subsListMutex.unlock();

	if (!eventSubList) {
		return false;
	}
	else {
		EventArgs emptyArgs;
		for (int subIndex = 0; subIndex < eventSubList->size(); subIndex++) {
			bool wasConsumed = eventSubList->at(subIndex)->Execute(emptyArgs);
			if (wasConsumed) return true;
		}
	}

	return true;
}


void EventSystem::ThrowError(std::string const& errorMsg) const
{
	ERROR_AND_DIE(errorMsg);
}

void SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunction functionPtr)
{
	if (g_theEventSystem) {
		g_theEventSystem->SubscribeEventCallbackFunction(eventName, functionPtr);
	}
}

void UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunction functionPtr)
{
	if (g_theEventSystem) {
		g_theEventSystem->UnsubscribeEventCallbackFunction(eventName, functionPtr);
	}
}

bool FireEvent(std::string const& eventName, EventArgs& args)
{
	if (g_theEventSystem) {
		return g_theEventSystem->FireEvent(eventName, args);
	}

	return false;
}

bool FireEvent(std::string const& eventName)
{
	if (g_theEventSystem) {
		return g_theEventSystem->FireEvent(eventName);
	}

	return false;
}


