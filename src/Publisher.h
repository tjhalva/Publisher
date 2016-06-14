#ifndef PUBLISHER_H
#define PUBLISHER_H

// System Headers
#include <functional>
#include <memory>
#include <list>

namespace tjh
{

//! A generic and lightweight publisher interface.  Shared pointer semantics
//! are used to manage the lifetime of a listener's callback object to permit
//! destruction of the any subscribers, which are heap allocated, at any point 
//! during a publication.
//! @tparam T The argument return type of the published event.
template<typename... T>
struct IPublisher
{
public:
	virtual void subscribe(std::weak_ptr<std::function<void(T...)> > callback) = 0;
	virtual ~IPublisher() {}
};

//! A concrete implementation of an IPublisher which exposes a "publish" method
//! to an owner through a templated friend.
//! @tparam Friend The class which owns the instance.
//! @tparam @see IPublisher.
template <class Friend, typename... T>
class Publisher : public IPublisher<T...>
{
public:
	//! The registration method for attaching a listener to an observer.
	//! @param callback The callback function which should be raised to the
	//!        listener when fired.
	//! @note There is no unsubscribe, the client should just null the weak_ptr
	virtual void subscribe(std::weak_ptr<std::function<void(T...)> > callback)
	{
		if (!callback.expired())
		{
			_callbacks.push_back(callback);
		}
	}

protected:
	friend Friend;

	//! A method used to propagate an object of type T to any subscribed listeners.
	//! @param args The argument to pass to any subscribed listeners.
	virtual void publish(T... args)
	{
		// Iterate through the callback list and remove stale pointers. This latent 
        // clean-up mechanism may be undesirable in some applications; an alternative
        // would be to have a destruction publication to trigger the initial publisher
        // to remove the callback from its internal list now, rather than at a latent
        // point in the future.
		auto callback = std::begin(_callbacks);
		while (callback != std::end(_callbacks))
		{
			if (callback->expired())
			{
				callback = _callbacks.erase(callback);
			}
			else
			{
				++callback;
			}
		}

		// Copy the list to enforce the construct that you cannot add a subscriber
		// while the event is firing as the ordering could be ambiguous and is
		// therefore not supported.
		auto callbacks = _callbacks;

		for (callback = std::begin(callbacks);
			 callback != std::end(callbacks);
			 ++callback)
		{
			if (auto ptr = callback->lock())
			{
				ptr->operator()(args...);
			}
		}
	}

	//! The callback list which contains weak_ptrs to listener callbacks. A 
    //! std::list is used because random access is not required and back
    //! insertions are guranteed to be constant time without reallocations.
	std::list<std::weak_ptr<std::function<void(T...)> > > _callbacks;
};

} // end tjh

#endif // PUBLISHER_H