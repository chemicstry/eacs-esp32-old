#include "RFIDService.h"
#include "PN532Instance.h"
#include "Utils.h"

using namespace std::placeholders;

RFIDService::RFIDService(JsonEventInterface& evtif): _evtif(evtif), _target(-1), _releaseTimer(-1)
{
    _evtif.On("RFIDService_TagTransceive", std::bind(&RFIDService::OnTagTransceive, this, _1));
    _evtif.On("RFIDService_ReleaseTarget", std::bind(&RFIDService::OnReleaseTarget, this, _1));
}

void RFIDService::OnTagTransceive(const JsonObject& args)
{
    // Check if we have target
    if (_target == -1)
    {
        // Send error
        StaticJsonBuffer<50> jsonBuffer;
        JsonObject& resp = jsonBuffer.createObject();
        resp["id"] = args["id"].as<uint32_t>(); // Same response id
        resp["error"] = "Target not active";
        _evtif.Send("RFIDService_TagTransceive", resp);

        return;
    }

    // Parse hex string to binary array
    std::string hexstr(args["data"].as<char*>());
    BinaryData buf = HexStringToBinaryData(hexstr);

    // Create interface with tag
    TagInterface tif = NFCEXT.CreateTagInterface(_target);

    // Send
    if (tif.Write(buf))
    {
        // Send error
        StaticJsonBuffer<50> jsonBuffer;
        JsonObject& resp = jsonBuffer.createObject();
        resp["id"] = args["id"].as<uint32_t>(); // Same response id
        resp["error"] = "Send failed";
        _evtif.Send("RFIDService_TagTransceive", resp);
        
        return;
    }

    // Receive
    if (tif.Read(buf) < 0)
    {
        // Send error
        StaticJsonBuffer<50> jsonBuffer;
        JsonObject& resp = jsonBuffer.createObject();
        resp["id"] = args["id"].as<uint32_t>(); // Same response id
        resp["error"] = "Receive failed";
        _evtif.Send("RFIDService_TagTransceive", resp);
        
        return;
    }

    // Build response packet
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& resp = jsonBuffer.createObject();
    resp["id"] = args["id"].as<uint32_t>(); // Same response id
    hexstr = BinaryDataToHexString(buf);
    resp["data"] = hexstr.c_str();
    _evtif.Send("RFIDService_TagTransceive", resp);
}

void RFIDService::OnReleaseTarget(const JsonObject& args)
{
    if (_releaseTimer != -1)
    {
        _timer.deleteTimer(_releaseTimer);
        _releaseTimer = -1;
    }

    _target = -1;
}

void RFIDService::AcquireNewTarget()
{
    // Finds nearby ISO14443 Type A tags
    InListPassiveTargetResponse resp = NFCEXT.InListPassiveTarget(1, BRTY_106KBPS_TYPE_A);

    // For parsing response data
    ByteBuffer buf(resp.TgData);

    // Only read one tag at a time
    if (resp.NbTg != 1)
        return;

    // Parse as ISO14443 Type A target
    PN532Packets::TargetDataTypeA tgdata;
    buf >> tgdata;

    // Save target id
    _target = tgdata.Tg;

    // Build new target packet
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& args = jsonBuffer.createObject();
    JsonArray& ATQA = args.createNestedArray("ATQA");
    ATQA.add(tgdata.ATQA[0]);
    ATQA.add(tgdata.ATQA[1]);
    std::string UID = BinaryDataToHexString(tgdata.UID);
    std::string ATS = BinaryDataToHexString(tgdata.ATS);
    args["SAK"] = tgdata.SAK;
    args["UID"] = UID.c_str();
    args["ATS"] = ATS.c_str();
    _evtif.Send("RFIDService_NewTarget", args);

    _releaseTimer = _timer.setTimeout(NEW_TARGET_TIMEOUT, [this]() {
        _target = -1;
    });
}

void RFIDService::Update()
{
    _timer.run();

    if (_target == -1)
        AcquireNewTarget();
}