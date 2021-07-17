#include "hardware.h"

std::map<int, Channel*> Channel::channels;

int pinlist[] = { 13, 12, 14, 25, 26, 25, 33, 32, 0 };

Channel dummyCh(0,0,Channel::PT_NONE);

void Channel::buildList()
{
    Channel* ch;

    for (int i = 0; pinlist[i]; i++)
    {
        ch = new Channel(i, pinlist[i], PT_ANALOG);
        channels[i] = ch;
    }
}

Channel* Channel::getChannel(uint8_t i)
{
    Channel *ch = Channel::channels[i];
    if (ch) return(ch);
    else
    {
        return &dummyCh;
    }
}