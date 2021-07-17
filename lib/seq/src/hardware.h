#pragma once

#ifdef ARDUINO
#include <Arduino.h>
#else

#define pinMode(pin, mode) ((void)0)
#define ledcAttachPin(ledch, mode) ((void)0)
#define digitalWrite(pin, value) ((void)0)
#define ledcWrite(ledch, val) ((void)0)

#endif

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

    Channel(uint8_t n, uint8_t p, pin_type t);

    uint32_t get()
    {
        return value;
    }
    void set(uint32_t v);
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