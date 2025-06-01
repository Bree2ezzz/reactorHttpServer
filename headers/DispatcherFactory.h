#ifndef DISPATCHERFACTORY_H
#define DISPATCHERFACTORY_H

#include <memory>
#include "Dispatcher.h"
#include "EventLoop.h"

class DispatcherFactory {
public:
    static std::shared_ptr<Dispatcher> createDispatcher(std::shared_ptr<EventLoop> evLoop);
};

#endif // DISPATCHERFACTORY_H 