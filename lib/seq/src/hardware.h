#pragma once

#include <Arduino.h>
#include <map>

class Channel
{
public:
    enum pin_type
    {
        PT_NONE,
        PT_ANALOG,
        PT_DIGITAL
    };
    Channel(uint8_t n, uint8_t p, pin_type t)
    {
        channelnum = n;
        pin = p;
        pt = t;
        switch (pt)
        {
        case PT_DIGITAL:
            pinMode(pin, OUTPUT);
            break;
        case PT_ANALOG:
            ledcAttachPin(pin, ledch);
            break;
        case PT_NONE:
            break;
        }
        value = 0;
    }
    void set(uint32_t v)
    {
        value = v;
        switch (pt)
        {
        case PT_DIGITAL:
            digitalWrite(pin, value);
            break;
        case PT_ANALOG:
            ledcWrite(ledch, v);
            break;
        case PT_NONE:
            break;

        }
    }
    uint32_t get()
    {
        return value;
    }
    bool valid() { return (pt != PT_NONE); }

    static void buildList();
    static Channel *getChannel(uint8_t);

private:
    uint8_t channelnum;
    uint8_t pin;
    pin_type pt;
    uint32_t value;
    uint8_t ledch;

    static std::map<int, Channel *> channels;
};