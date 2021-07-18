#include "hardware.h"

std::map<int, Channel *> Channel::channels;

int pinlist[] = {13, 12, 14, 27, 26, 25, 33, 32, 0};

Channel dummyCh(0, 0, Channel::PT_NONE);

#ifdef ARDUINO

Channel::Channel(uint8_t n, uint8_t p, pin_type t)
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
        ledch = n;
        ledcSetup(ledch, 5000, 10);
        ledcAttachPin(pin, ledch);
        break;
    case PT_NONE:
        break;
    }
}

void Channel::set(uint32_t v)
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

#else

Channel::Channel(uint8_t n, uint8_t p, pin_type t)
{
}

void Channel::set(uint32_t v)
{
}

#endif

Channel *Channel::getChannel(uint8_t i)
{
    Channel *ch = Channel::channels[i];
    if (ch)
    {
        return (ch);
    }
    else
    {
        return &dummyCh;
    }
}

void Channel::buildList()
{
    for (int i = 0; pinlist[i]; i++)
    {
        Channel* ch = new Channel(i, pinlist[i], PT_ANALOG);
        channels[i] = ch;
    }
}