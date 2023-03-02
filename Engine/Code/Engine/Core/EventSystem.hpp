#pragma once
#include <vector>
#include <map>
#include <string>
#include <mutex>

class NamedProperties;

typedef NamedProperties EventArgs;
typedef bool (*EventCallbackFunction)(EventArgs& args);

class EventSubscription {
public:
	EventSubscription() = default;
	virtual ~EventSubscription() = default;

	virtual bool IsSameFunction(void* otherFunction) const = 0;
	virtual bool Execute(EventArgs&) const = 0;
	virtual bool BelongsToObject(void* object) const { object; return false; }
};

class EventFuncSubscription : public EventSubscription {
public:
	EventFuncSubscription(EventCallbackFunction callbackFunction);;
	EventCallbackFunction m_callbackFunction;
	virtual bool Execute(EventArgs& eventArgs) const override;
	virtual bool IsSameFunction(void* otherFunction) const override;
};

template<typename T_ObjectType>
class EventMethodSubscription : public EventSubscription {
	friend class EventSystem;
	typedef bool(T_ObjectType::* MethodType)(EventArgs& args);
public:
	EventMethodSubscription(T_ObjectType* objectInstance, MethodType subscribedMethod) :
		m_objectInstance(objectInstance),
		m_method(subscribedMethod) {}

	virtual bool Execute(EventArgs& eventArgs) const override {
		return ((*m_objectInstance).*m_method)(eventArgs);
	}

	virtual bool IsSameFunction(void* otherFunction) const override {
		MethodType* otherFuncAsMethodType = reinterpret_cast<MethodType*>(otherFunction);
		return (m_method) == *otherFuncAsMethodType;
	}

	virtual bool BelongsToObject(void* object) const override {
		return m_objectInstance == object;
	}


private:
	T_ObjectType* m_objectInstance = nullptr;
	MethodType m_method = nullptr;
};

struct EventSystemConfig {

};


extern EventSystem* g_theEventSystem;

typedef std::vector<EventSubscription*> SubscriptionList;

class EventSystem {
public:
	EventSystem(EventSystemConfig const& config);
	~EventSystem();
	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

	void GetRegisteredEventNames(std::vector< std::string >& outNames) const;

	template<typename T_ObjectType, typename MethodType>
	inline void SubscribeEventCallbackFunction(std::string const& eventName, T_ObjectType* objectInstance, MethodType functionPtr);
	template<typename T_ObjectType, typename MethodType>
	inline void UnsubscribeEventCallbackFunction(std::string const& eventName, T_ObjectType* objectInstance, MethodType functionPtr);
	template<typename T_ObjectType, typename MethodType> // Unsubscribe specific object's methods from all events
	inline void UnsubscribeAllEventCallbackFunctions(T_ObjectType* objectInstance, MethodType functionPtr);
	template<typename T_ObjectType> // Unsubscribe object's methods from all events
	inline void UnsubscribeAllEventCallbackFunctions(T_ObjectType* objectInstance);

	void SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunction functionPtr);
	void UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunction functionPtr);

	bool FireEvent(std::string const& eventName, EventArgs& args);
	bool FireEvent(std::string const& eventName);
	void ThrowError(std::string const& errorMsg) const;

protected:
	EventSystemConfig m_config;

	mutable std::mutex m_subsListMutex;
	std::map<std::string, SubscriptionList> m_subscriptionListByEventName;
};

template<typename T_ObjectType, typename MethodType>
void EventSystem::SubscribeEventCallbackFunction(std::string const& eventName, T_ObjectType* objectInstance, MethodType functionPtr)
{
	EventMethodSubscription<T_ObjectType>* newSubscription = new EventMethodSubscription<T_ObjectType>(objectInstance, functionPtr);

	m_subsListMutex.lock();

	std::map<std::string, SubscriptionList>::iterator iter = m_subscriptionListByEventName.find(eventName);
	if (iter == m_subscriptionListByEventName.end()) {
		SubscriptionList eventSubList;
		eventSubList.push_back(newSubscription);
		m_subscriptionListByEventName[eventName] = eventSubList;
	}
	else {
		SubscriptionList& eventSubList = iter->second;
		for (int subIndex = 0; subIndex < eventSubList.size(); subIndex++) {
			if (eventSubList[subIndex]->IsSameFunction(&functionPtr)) {
				ThrowError("THERE WAS AN ATTEMPT TO DOUBLE SUBSCRIBE A FUNCTION TO AN EVENT");
			}
		}
		eventSubList.push_back(newSubscription);
	}

	m_subsListMutex.unlock();
}

template<typename T_ObjectType, typename MethodType>
void EventSystem::UnsubscribeEventCallbackFunction(std::string const& eventName, T_ObjectType* objectInstance, MethodType functionPtr)
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
			EventSubscription*& eventSub = eventSubList[subIndex];
			if (eventSub->BelongsToObject(objectInstance)) {
				if (eventSubList[subIndex]->IsSameFunction(&functionPtr)) {
					delete eventSubList[subIndex];
					eventSubList.erase(eventSubList.begin() + subIndex);
					m_subsListMutex.unlock();
					return;
				}
			}
		}
	}

	m_subsListMutex.unlock();
}


template<typename T_ObjectType, typename MethodType>
void EventSystem::UnsubscribeAllEventCallbackFunctions(T_ObjectType* objectInstance, MethodType functionPtr)
{
	for (auto it = m_subscriptionListByEventName.begin(); it != m_subscriptionListByEventName.end(); it++) {
		SubscriptionList& subList = it->second;
		if (subList.size() == 0) continue;
		m_subsListMutex.lock();

		for (auto subListIt = subList.begin(); subListIt != subList.end();) {
			EventSubscription*& eventSub = *subListIt;
			if (eventSub->BelongsToObject(objectInstance)) {
				if (eventSub->IsSameFunction(&functionPtr)) {
					subListIt = subList.erase(subListIt);
				}
				else {
					subListIt++;
				}
			}
			else {
				subListIt++;
			}
		}

		m_subsListMutex.unlock();
	}
}

template<typename T_ObjectType>
void EventSystem::UnsubscribeAllEventCallbackFunctions(T_ObjectType* objectInstance)
{
	for (auto it = m_subscriptionListByEventName.begin(); it != m_subscriptionListByEventName.end(); it++) {
		SubscriptionList& subList = it->second;
		if (subList.size() == 0) continue;
		m_subsListMutex.lock();

		for (auto subListIt = subList.begin(); subListIt != subList.end();) {
			EventSubscription*& eventSub = *subListIt;
			if (eventSub->BelongsToObject(objectInstance)) {
				subListIt = subList.erase(subListIt);
			}
			else {
				subListIt++;
			}
		}

		m_subsListMutex.unlock();
	}
}

template<typename T_ObjectType, typename MethodType>
void SubscribeEventCallbackFunction(std::string const& eventName, T_ObjectType* objectInstance, MethodType functionPtr)
{
	if (g_theEventSystem) {
		g_theEventSystem->SubscribeEventCallbackFunction(eventName, objectInstance, functionPtr);
	}
}

template<typename T_ObjectType, typename MethodType>
void UnsubscribeEventCallbackFunction(std::string const& eventName, T_ObjectType* objectInstance, MethodType functionPtr)
{
	if (g_theEventSystem) {
		g_theEventSystem->UnsubscribeEventCallbackFunction(eventName, objectInstance, functionPtr);
	}
}

template<typename T_ObjectType, typename MethodType>
void UnsubscribeAllEventCallbackFunctions(T_ObjectType* objectInstance, MethodType functionPtr)
{
	if (g_theEventSystem) {
		g_theEventSystem->UnsubscribeAllEventCallbackFunctions(objectInstance, functionPtr);
	}
}

template<typename T_ObjectType>
void UnsubscribeAllEventCallbackFunctions(T_ObjectType* objectInstance)
{
	if (g_theEventSystem) {
		g_theEventSystem->UnsubscribeAllEventCallbackFunctions(objectInstance);
	}
}

void SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunction functionPtr);
void UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunction functionPtr);
bool FireEvent(std::string const& eventName, EventArgs& args);
bool FireEvent(std::string const& eventName);


class EventRecipient {
public:
	virtual ~EventRecipient() {
		UnsubscribeAllEventCallbackFunctions(this);
	}
};