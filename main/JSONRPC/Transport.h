#ifndef _JSONRPCTRANSPORT_H_
#define _JSONRPCTRANSPORT_H_

#include <json.hpp>
#include <functional>
#include <string>

using json = nlohmann::json;

namespace JSONRPC
{
    class NoTransportHandlerException : public std::exception
    {
        const char* what() const throw()
        {
            return "Attempt to send without a valid transport handler";
        }
    };

    typedef std::function<void(const json&)> JSONTransportHandler;
    typedef std::function<void(const std::string&)> StringTransportHandler;

    class Transport
    {
    public:
        // Registers a function which handles outgoing json data
        void SetHandler(const JSONTransportHandler& handler);
        void SetHandler(const StringTransportHandler& handler);

        // Handle incoming json data
        virtual void HandleData(const json& j) = 0;
        void HandleData(const std::string& s);

    protected:
        // Send data to transport
        void Send(const json& j);

    private:
        JSONTransportHandler _JSONHandler;
        StringTransportHandler _StringHandler;
    };
}

#endif
