#ifndef EXAMPLE_H
#define EXAMPLE_H

// tjh Headers
#include "../src/Publisher.h"

// System Headers
#include <functional>
#include <memory>
#include <iostream>
#include <string>

namespace tjh
{
namespace example
{

//! A listener class which subscribes to an exterior publication.
class Client
{
public:
    //! @note: There is no constraint requiring an IPublisher to be passed into the
    //! constructor of a listener, this pattern was simply used to demonstrate the 
    //! mechanism of subscription. The shared_ptr callback and the IPublisher object
    //! just, at some point, need to exist in the same scope.
    Client(IPublisher<int, const std::string&>& publisher)
    {
        // If the Client is going to subsribe to publications, it needs to create
        // a callback object. This callback object is a shared_ptr owned by the 
        // Client; the callback will have the same life-span as the naked
        // "this" created within the bound function. This ensures that the
        // bound function is either valid, or its parent shared_ptr has
        // already been destructed because the object itself no longer exists.
        _callback =
            tjh::make_callback(&Client::handler, this);

        // The shared_ptr is provided to the IPublisher interface, and the IPublisher
        // interface holds a weak_ptr that is checked for validity before the callback is
        // fired.
        publisher.subscribe(_callback);
    }

private:
    void handler(int a, const std::string& b)
    {
        // Published arguments!
        std::cout << __FUNCTION__ << ": a=" << a << ", b=" << b << std::endl;

        // Upon receiving a publication from the subscriber, the listener has determined
        // it no longer needs to exist, so it deletes itself. This deletion nulls the
        // weak_ptr held in the IPublisher object which will be cleaned up during the
        // next publication.
        // @note: The below line ONLY works if the object was heap allocated; if the 
        //        object was allocated on the stack, a double deletion will occur.
        delete this;
    }

    // The callback object enforces the requirement that this instance's lifetime is the
    // same as the contained function's lifetime.
    std::shared_ptr<std::function<void(int, const std::string&)>> _callback;
};

class Primary : public IPublisher<int, const std::string&>
{
public:
    virtual void subscribe(std::weak_ptr<std::function<void(int, const std::string&)> > callback)
    {
        // This is a simple pass through to the composed publisher.
        _publisher.subscribe(callback);
    }

    void doSomething(int a, const std::string& b)
    {
        // Publish the contents of a and b to any listeners.
        _publisher.publish(a, b);
    }

private:
    tjh::Publisher<Primary, int, const std::string&> _publisher;
};

} // end example
} // end tjh

#endif // EXAMPLE_H