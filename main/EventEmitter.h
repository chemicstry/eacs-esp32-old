#ifndef _EVENTEMITTER_H_
#define _EVENTEMITTER_H_

#include <map>
#include <vector>
#include <string>
#include <functional>

template<typename T>
class EventEmitter
{
public:
    typedef std::function<void(const T&)> EventCb;

    void On(const std::string& name, const EventCb& listener)
    {
        _events[name].push_back(listener);
    }

    void Emit(const std::string& name, const T& data)
    {
        auto event = _events.find(name);

        if (event != _events.end())
        {
            // Fire all callbacks
            for (EventCb& cb : event->second)
                cb(data);
        }
    }

private:
    std::map<std::string, std::vector<EventCb>> _events;
};

#endif
