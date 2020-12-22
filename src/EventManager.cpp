#include <EventManager.hpp>

#include <deque>
#include <map>
#include <algorithm>

class ListenerContainer {
    public :
        int repeatCount;
        bool isForever;
        Evt::ListenerId listenerId;
        Evt::EvCallback listener;

        int currentRepeatCount;
        bool isEnabled = true;

        ListenerContainer(
            int repeatCount,
            bool isForever,
            Evt::ListenerId listenerId,
            Evt::EvCallback listener
        ) {
            this -> repeatCount = repeatCount;
            this -> currentRepeatCount = repeatCount;
            this -> isForever = isForever;
            this -> listenerId = listenerId;
            this -> listener = listener;
        }
};
typedef std::map<std::string, std::deque<ListenerContainer*>>::iterator ContainerIterator;

class EventManager {
    public :
        Evt::ListenerId lastListenerId = 1;
        std::map<std::string, std::deque<ListenerContainer*>> containerMap;
        std::deque<std::pair<std::string, Evt::EvCallback*>*> listenersWaitingToRun;

        Evt::ListenerId generateId() { return lastListenerId++; }
        int getIndexOfSpecificContainerListById(std::deque<ListenerContainer*> containerList, Evt::ListenerId listenerId) {
            for(int index = 0 ; index < containerList.size() ; index++) 
                if(containerList.at(index) -> listenerId == listenerId) return index;
            return -1;
        }
        Evt::ListenerId addNewEventListener(bool prepend, std::initializer_list<std::string> events, int repeatCount, bool isForever, Evt::EvCallback listener) {
            Evt::ListenerId listenerId = generateId();
            ListenerContainer* listenerContainer = new ListenerContainer(
                repeatCount,
                isForever,
                listenerId,
                listener
            );
            for_each(events.begin(), events.end(), [this, listenerContainer, prepend](std::string eachEvent) {
                ContainerIterator containerEntry = containerMap.find(eachEvent);
                if(containerEntry == containerMap.end()) containerMap.insert(std::make_pair(eachEvent, std::deque<ListenerContainer*>{ listenerContainer }));
                else {
                    if(prepend) containerEntry -> second.push_front(listenerContainer);
                    else containerEntry -> second.push_back(listenerContainer);
                }
            });
            return listenerId;
        }

};

EventManager* eventManager = new EventManager();
#include<Arduino.h>
class EventDataPair {
    public :
        std::string event;
        std::string data;
        EventDataPair(std::string event, std::string data) {
            this -> event = event;
            this -> data = data;
        }
};
std::deque<EventDataPair*> eventQueue;
void Evt::run() {
    while(eventQueue.size()) {
        EventDataPair* eventDataPair = eventQueue.at(0);
        eventQueue.pop_front();
        ContainerIterator eventListenerContainerEntry = eventManager -> containerMap.find(eventDataPair -> event);
        std::deque<Evt::ListenerId> idsToRemove;
        if(eventListenerContainerEntry != eventManager -> containerMap.end() && eventListenerContainerEntry -> second.size() > 0) {
            for_each(
                eventListenerContainerEntry -> second.begin(),
                eventListenerContainerEntry -> second.end(),
                [&idsToRemove, eventDataPair](ListenerContainer* eachListenerContainer){
                    if(eachListenerContainer -> isEnabled) {
                        Evt::EvSignal signal = (eachListenerContainer -> listener)(eventDataPair -> data);
                        if(signal == Evt::BREAK) Evt::remove(eachListenerContainer -> listenerId);
                        else {
                            if(!eachListenerContainer -> isForever) eachListenerContainer -> currentRepeatCount--;
                            if(eachListenerContainer -> currentRepeatCount == 0) idsToRemove.push_back(eachListenerContainer -> listenerId);
                        }
                    }
                }
            );
        }
        for(Evt::ListenerId eachIdToRemove : idsToRemove) Evt::remove(eachIdToRemove);
        delete eventDataPair;
    }
}

Evt::ListenerId Evt::on(std::initializer_list<std::string> events, Evt::EvCallback listener) { return eventManager -> addNewEventListener(false, events, -1, true, listener); }
Evt::ListenerId Evt::onPrepend(std::initializer_list<std::string> events, Evt::EvCallback listener) { return eventManager -> addNewEventListener(true, events, -1, true, listener); }

Evt::ListenerId Evt::once(std::initializer_list<std::string> events, Evt::EvCallback listener) { return eventManager -> addNewEventListener(false, events, 1, false, listener); }
Evt::ListenerId Evt::oncePrepend(std::initializer_list<std::string> events, Evt::EvCallback listener) { return eventManager -> addNewEventListener(true, events, 1, false, listener); }

Evt::ListenerId Evt::multiple(std::initializer_list<std::string> events, int repeatCount, Evt::EvCallback listener) { return eventManager -> addNewEventListener(false, events, repeatCount, false, listener); }
Evt::ListenerId Evt::multiplePrepend(std::initializer_list<std::string> events, int repeatCount, Evt::EvCallback listener) { return eventManager -> addNewEventListener(true, events, repeatCount, false, listener); }

void Evt::emit(std::string event, std::string dataString) {
    eventQueue.push_back(new EventDataPair(event, dataString));
    /*
    ContainerIterator containerEntry = eventManager -> containerMap.find(event);
    if(containerEntry != eventManager -> containerMap.end() && containerEntry -> second.size() > 0) {
        for_each(containerEntry -> second.begin(), containerEntry -> second.end(), [dataString](ListenerContainer* eachListenerContainer){
            if(eachListenerContainer -> isEnabled) {

            }
        });
    }
    */
}

void Evt::remove(const Evt::ListenerId listenerId) {
    for(ContainerIterator eachMapIterator = eventManager -> containerMap.begin() ; eachMapIterator != eventManager -> containerMap.end() ; eachMapIterator++) {
        int indexToRemove = eventManager -> getIndexOfSpecificContainerListById(eachMapIterator -> second, listenerId);
        if(indexToRemove != -1) {
            ListenerContainer* containerToRemove = eachMapIterator -> second.at(indexToRemove);
            eachMapIterator -> second.erase(eachMapIterator -> second.begin() + indexToRemove);
            if(getListenerCount(listenerId) == 0) delete containerToRemove;
        }
    }
}

void Evt::removeOn(const Evt::ListenerId listenerId, std::initializer_list<std::string> events) {
    for(ContainerIterator eachMapIterator = eventManager -> containerMap.begin() ; eachMapIterator != eventManager -> containerMap.end() ; eachMapIterator++) {
        if(std::find(events.begin(), events.end(), eachMapIterator -> first) != events.end()) {
            int indexToRemove = eventManager -> getIndexOfSpecificContainerListById(eachMapIterator -> second, listenerId);
            if(indexToRemove != -1) {
                ListenerContainer* containerToRemove = eachMapIterator -> second.at(indexToRemove);
                eachMapIterator -> second.erase(eachMapIterator -> second.begin() + indexToRemove);
                if(getListenerCount(listenerId) == 0) delete containerToRemove;
            }
        }
    }
}

void Evt::removeAllOn(std::initializer_list<std::string> events) {
    for(ContainerIterator eachMapIterator = eventManager -> containerMap.begin() ; eachMapIterator != eventManager -> containerMap.end() ; eachMapIterator++) {
        if(std::find(events.begin(), events.end(), eachMapIterator -> first) != events.end()) {
            std::deque<ListenerContainer*> tmpContainer;
            for(ListenerContainer* eachContainer : eachMapIterator -> second) tmpContainer.push_back(eachContainer);
            for(ListenerContainer* eachContainer : tmpContainer) {
                int indexToRemove = eventManager -> getIndexOfSpecificContainerListById(eachMapIterator -> second, eachContainer -> listenerId);
                if(indexToRemove != -1) {
                    eachMapIterator -> second.erase(eachMapIterator -> second.begin() + indexToRemove);
                    if(getListenerCount(eachContainer -> listenerId) == 0) delete eachContainer;
                }
            }
        }
    }
}

bool Evt::isEnabled(const Evt::ListenerId listenerId) {
    for(ContainerIterator eachMapIterator = eventManager -> containerMap.begin() ; eachMapIterator != eventManager -> containerMap.end() ; eachMapIterator++) {
        int indexToCheck = eventManager -> getIndexOfSpecificContainerListById(eachMapIterator -> second, listenerId);
        if(indexToCheck != -1) return eachMapIterator -> second.at(indexToCheck) -> isEnabled;
    }
    return false;
}

void Evt::enable(const Evt::ListenerId listenerId) {
    for(ContainerIterator eachMapIterator = eventManager -> containerMap.begin() ; eachMapIterator != eventManager -> containerMap.end() ; eachMapIterator++) {
        int indexToModify = eventManager -> getIndexOfSpecificContainerListById(eachMapIterator -> second, listenerId);
        if(indexToModify != -1) eachMapIterator -> second.at(indexToModify) -> isEnabled = true;
    }
}

void Evt::disable(const Evt::ListenerId listenerId) {
    for(ContainerIterator eachMapIterator = eventManager -> containerMap.begin() ; eachMapIterator != eventManager -> containerMap.end() ; eachMapIterator++) {
        int indexToModify = eventManager -> getIndexOfSpecificContainerListById(eachMapIterator -> second, listenerId);
        if(indexToModify != -1) eachMapIterator -> second.at(indexToModify) -> isEnabled = false;
    }
}

void Evt::toggle(const Evt::ListenerId listenerId) {
    for(ContainerIterator eachMapIterator = eventManager -> containerMap.begin() ; eachMapIterator != eventManager -> containerMap.end() ; eachMapIterator++) {
        int indexToModify = eventManager -> getIndexOfSpecificContainerListById(eachMapIterator -> second, listenerId);
        if(indexToModify != -1) {
            ListenerContainer* listenerContainer = eachMapIterator -> second.at(indexToModify);
            listenerContainer -> isEnabled = !listenerContainer -> isEnabled;
        }
    }
}

int Evt::getListenerCount(const Evt::ListenerId listenerId) {
    int count = 0;
    for(ContainerIterator eachMapIterator = eventManager -> containerMap.begin() ; eachMapIterator != eventManager -> containerMap.end() ; eachMapIterator++)
        if(eventManager -> getIndexOfSpecificContainerListById(eachMapIterator -> second, listenerId) != -1) count++;
    return count;
}

int Evt::getListenerCountOn(std::string event) {
    ContainerIterator mapIterator = eventManager -> containerMap.find(event);
    if(mapIterator != eventManager -> containerMap.end() && mapIterator -> second.size() > 0) return mapIterator -> second.size();
    return 0;
}