#include "Transport.h"

using namespace JSONRPC;
using json = nlohmann::json;

// Registers a function which handles outgoing json data
void Transport::SetHandler(const JSONTransportHandler& handler)
{
    _JSONHandler = handler;
}

void Transport::SetHandler(const StringTransportHandler& handler)
{
    _StringHandler = handler;
}

void Transport::HandleData(const std::string& s)
{
    HandleData(json::parse(s));
}

void Transport::Send(const json& j)
{
    // Prefer json handler
    if (_JSONHandler)
        _JSONHandler(j);
    else if (_StringHandler)
        _StringHandler(j.dump());
    else
        throw NoTransportHandlerException();
}