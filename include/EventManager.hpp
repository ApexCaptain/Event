#pragma once
#ifndef EVENT_MANAGER_HPP
#define EVENT_MANAGER_HPP

#include <string>
#include <initializer_list>
#include <functional>

namespace Evt {

    enum EvSignal {
        CONTINUE = 0,
        BREAK
    };

    typedef std::function<EvSignal(std::string dataString)> EvCallback;
    typedef unsigned long ListenerId;

    void run();

    ListenerId on(std::initializer_list<std::string> events, EvCallback listener);
    ListenerId onPrepend(std::initializer_list<std::string> events, EvCallback listener);
    ListenerId once(std::initializer_list<std::string> events, EvCallback listener);
    ListenerId oncePrepend(std::initializer_list<std::string> events, EvCallback listener);
    ListenerId multiple(std::initializer_list<std::string>, int repeatCount, EvCallback listener);
    ListenerId multiplePrepend(std::initializer_list<std::string>, int repeatCount, EvCallback listener);

    void emit(std::string event, std::string dataString);
    
    void remove(const ListenerId listenerId);
    void removeOn(const ListenerId listenerId, std::initializer_list<std::string> events);
    void removeAllOn(std::initializer_list<std::string> events);
    bool isEnabled(const ListenerId listenerId);
    void enable(const ListenerId listenerId);
    void disable(const ListenerId listenerId);
    void toggle(const ListenerId listenerId);
    int getListenerCount(const ListenerId listenerId);
    int getListenerCountOn(std::string event);

}

#endif