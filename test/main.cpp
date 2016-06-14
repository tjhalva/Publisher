#include "Example.h"

int main()
{
    tjh::example::Primary primary;
    auto* pClient = new tjh::example::Client(primary);

    primary.doSomething(1, std::string("2"));

    // Expected Output: 
    // tjh::example::Client::handler: a=1, b=2
}