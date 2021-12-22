#include "yolandapp/channel.hpp"
#include "yolandapp/event_loop.hpp"

// namespace yolandapp {


int channel::write_event_enable() 
{
    event_loop *eventLoop = (event_loop *) this->data;
    this->events = this->events | EVENT_WRITE;
    eventLoop->update_channel_event(this->fd, this);
    return 0;
}

int channel::write_event_disable()
{
    event_loop *eventLoop = (event_loop *) this->data;
    this->events = this->events & ~EVENT_WRITE;
    eventLoop->update_channel_event(this->fd, this);
    return 0;
}

    
// } // namespace yolandapp

