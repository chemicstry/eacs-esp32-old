#include "Client.h"
#include <vector>

using namespace JSONRPC;

Client::Client(): _id(0)
{
}

void Client::HandleData(const json& j)
{
    auto jsonit = j.find("jsonrpc");
    if (jsonit == j.end() || *jsonit != "2.0")
        throw InvalidJSONRPCException();

    uint32_t id = j["id"];

    // Find promise
    auto it = _promises.find(id);
    if (it == _promises.end())
        throw PromiseNotFoundException();

    auto promise = it->second;
    _promises.erase(it);

    auto errorit = j.find("error");
    if (errorit != j.end()) {
        auto error = *errorit;

        try {
            throw JSONRPCErrorException(error["code"], error["message"], error["data"]);
        } catch (const JSONRPCErrorException& e) {
            promise->set_exception(std::current_exception());
        }
    } else
        promise->set_value(j["result"]);
}

