#ifndef _DATAINTERFCAE_H_
#define _DATAINTERFACE_H_

#include <functional>

template<typename T>
class DataInterface
{
private:
    typedef std::function<void(const T&)> DataCb;
    DataCb& _rxcb;
    DataCb& _txcb;

public:
    DataInterface(DataCb& rxcb, DataCb& txcb): _rxcb(rxcb), _txcb(txcb) {}

    void SetCb(DataCb rxcb)
    {
        _rxcb = rxcb;
    }

    void Send(const T& data)
    {
        if (_txcb)
            _txcb(data);
    }
};

template<typename T>
class BidirectionalDataInterface
{
private:
    typedef std::function<void(const T&)> DataCb;
    DataCb _downstreamCb, _upstreamCb;

public:
    BidirectionalDataInterface(): Downstream(_downstreamCb, _upstreamCb), Upstream(_upstreamCb, _downstreamCb) {}

    DataInterface<T> Downstream, Upstream;
};

#endif
