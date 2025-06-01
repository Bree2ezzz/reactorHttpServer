#include "../headers/DispatcherFactory.h"

#ifdef _WIN32
#include "../headers/IocpDispatcher.h"
#else
#include "../headers/EpollDispatcher.h"
#endif

std::shared_ptr<Dispatcher> DispatcherFactory::createDispatcher(std::shared_ptr<EventLoop> evLoop)
{
#ifdef _WIN32
    return std::make_shared<IocpDispatcher>(evLoop);
#else
    return std::make_shared<EpollDispatcher>(evLoop);
#endif
} 