# Publisher : A C++ Publisher/Subscriber Variadic Implementation

The Publisher<T, Args...> class and its associated IPublisher<Args...> interface form the basis of a simple and lightweight event registration mechanism permitting the implementation of the [Observer Pattern](https://en.wikipedia.org/wiki/Observer_pattern). There are no dependencies beyond C++11 or the (Boost C++ libraries.)[http://www.boost.org/] 

The entire class is provided inline in a single templated header, Publisher.h.  The class makes extensive usage of smart pointer semantics to manage the lifetime of subscribed listeners, and in certain cases, the ability for listeners to delete themselves in response to a publication.  

#Functionality and Behavior
The basic premise of the Publisher class is to facilitate the implementation of the Observer pattern. A Client class may wish to be informed of changes to an external object. This desire is implemented via a simple public Publisher::subscribe() method which takes a std::function<Args...> as an argument. The Client is responsible for creating an internal callback object and passing that callback object into the Publisher::subscribe() method.  The callback object is a shared_ptr to a function<void<Args...>. By wrapping the function object in a shared_ptr, the lifetime of the bound instance method is assured to be valid so long as the shared_ptr, which is owned by the instance, is valid.  The Publisher<T, Args...> maintains an internal list of callback objects, via weak_ptr.  When data is published, the Publisher will iterate through the callback list, first purging stale callbacks, and then secondly calling into the listeners to provide the associated data.

Upon receiving a publication from an IPublisher<Args...>, a heap created listener has the capability to delete itself. This self-deletion will null the weak_ptr held in the Publisher<Args...>'s callback list.

The latent clean-up mechanism may be undesirable in some applications; an alternative would be to add a second publication, fired from the destructor, to trigger the initial publisher to remove the callback from its internal list immediately, rather than at an undefined point in the future. In practice, the size of the internal callback lists are usually small so the purging traversal time is negligible.

#Quick Start
## Supported Platforms

The Publisher.h file has been explicitly tested on the following platforms and compilers supporting C++11:

* Windows using Visual Studio 2013
* Windows using Visual Studio 2015
* GNU/Linux using Clang/LLVM 3.6
* GNU/Linux using GCC 4.8.3

The inclusion of boost, version 1.60, has also been tested successfully on the above platforms outside of C++11.

## Using the Library

The Publisher/Subscriber pattern inherently requires the existence of both a publisher object, which provides data to listeners, and a listener object which registers for information with a publisher. A simple implemented IPublisher<Args...> would look something like below; take note that the "Primary" class implements the IPublisher<Args...> interface, and then delegates the implementation to the concrete and internal Publisher<T, Args...> object. The first template argument of the Publisher<T,Args...> class should be the type of the instance using the Publisher; this type is a templated friend which makes the publish() method visible to the class responsible for doing the publishing:

        class Primary : public tjh::IPublisher<int, const std::string&>
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

The subscriber builds a callback object in its constructor, and then passes it's callback to the publisher. A listener can be implemented in about 5 lines, but there are certain constraints the need to be understood about the lifetime and owners of certain objects. If a Client wishes to delete itself, it *MUST* be heap allocated otherwise a double deletion will occur. There are no constraints in place to enforce the ability to self delete; a potentional solution to enforce heap creation would be to create a public factory method while making the constructor private.

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
                std::shared_ptr<std::function<void(int, const std::string&)>>(
                    new std::function<void(int, const std::string&)>(
                        std::bind(
                            &Client::handler,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2)));

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
        
An example program has been provided in the /test directory which includes the code presented above with all the correct includes and namespaces.