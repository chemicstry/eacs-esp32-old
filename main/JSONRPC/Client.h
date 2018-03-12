#ifndef _JSONRPCCLIENT_H_
#define _JSONRPCCLIENT_H_

#include "Transport.h"
#include <json.hpp>
#include <string>
#include <future>
#include <map>

using json = nlohmann::json;

namespace JSONRPC
{
    class InvalidJSONRPCException : public std::exception
    {
        const char* what() const throw()
        {
            return "Invalid JSON RPC format";
        }
    };

    class PromiseNotFoundException : public std::exception
    {
        const char* what() const throw()
        {
            return "Promise not found";
        }
    };

    class JSONRPCErrorException : public std::exception
    {
        int32_t _code;
        std::string _message;
        json _data;

    public:
        JSONRPCErrorException(int32_t code, const std::string& message, const json& data): _code(code), _message(message), _data(data)
        {
        }

        int32_t code() const { return _code; }
        json data() const { return _data; }
        const char* what() const throw()
        {
            return _message.c_str();
        }
    };

    typedef std::shared_ptr<std::promise<json>> PromisePtr;
    typedef std::future<json> Future;

    class Client : public Transport
    {
    public:
        Client();

        template<typename ...Args>
        json Call(std::string method, Args&&... args);
        template<typename ...Args>
        Future CallAsync(std::string method, Args&&... args);

        // Handle incoming json data
        void HandleData(const json& j);

    private:
        uint32_t _id;
        std::map<uint32_t, PromisePtr> _promises;
    };

    template<typename Arg>
    inline void unpack(json& params, Arg&& arg)
    {
        params.push_back(arg);
    }

    template<typename Arg, typename ...Args>
    inline void unpack(json& params, Arg&& arg, Args&&... args)
    {
        params.push_back(arg);
        unpack(params, std::forward<Args>(args)...);
    }

    template<typename ...Args>
    inline Future Client::CallAsync(std::string method, Args&&... args)
    {
        uint32_t id = _id++;
        json j;

        j["jsonrpc"] = "2.0";
        j["method"] = method;
        j["id"] = id;

        // Unpack arguments
        unpack(j["params"], std::forward<Args>(args)...);

        // Save promise for matching response message to request
        auto promise = std::make_shared<std::promise<json>>();
        _promises[id] = promise;

        // Send formatted JSON RPC message
        Send(j);

        return promise->get_future();
    }
}

#endif
