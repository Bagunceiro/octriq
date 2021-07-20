#include "hardware.h"

std::map<int, Channel *> Channel::channels;

int pinlist[] = {13, 12, 14, 27, 26, 25, 33, 32, 0};

Channel dummyCh(0, 0, Channel::PT_NONE);

#ifdef ARDUINO

static const double factor = pow(255.0, (1.0 / 1023.0));

int Channel::powtable[1024];

void Channel::mkpowtable()
{
    for (int v = 0; v < 1024; v++)
    {
        // This is too expensive in processing time to do it on the fly every time
        powtable[v] = round(pow(factor, v));
    }
}

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
        ledcSetup(ledch, 5000, 8);
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
    {
        int vexp = 0;
        if ((v > 0) && (v < 1024))
        {
            vexp = powtable[v];
            ledcWrite(ledch, vexp);
        }
    }
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
        Channel *ch = new Channel(i, pinlist[i], PT_ANALOG);
        channels[i] = ch;
    }
    mkpowtable();
}