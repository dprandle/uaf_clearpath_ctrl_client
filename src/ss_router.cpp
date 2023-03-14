#include <stdarg.h>
#include "ss_router.h"

ss_router::ss_router() : connections()
{}

ss_router::ss_router(const ss_router &) : connections()
{}

ss_router &ss_router::operator=(const ss_router &)
{
    return *this;
}

ss_router::~ss_router()
{
    ss_disconnect_all(this);
}

sizet ss_disconnect(ss_connection_base *connection)
{
    void *casted_sig = connection->sig_addr();
    auto fiter = connection->router->connections.find(casted_sig);
    if (fiter != connection->router->connections.end()) {
        auto slot_iter = fiter->second.begin();
        while (slot_iter != fiter->second.end()) {
            if (slot_iter->get() == connection) {
                if (fiter->second.size() > 1) {
                    fiter->second.erase(slot_iter);
                }
                else {
                    connection->router->connections.erase(fiter);
                }
                return 1;
            }
            ++slot_iter;
        }
    }
    return 0;
}

void debug_log_connections(ss_router *router, const std::string &prefix)
{
    const char *pf_str = prefix.c_str();
    dlog("%s router addr:%p  signals connected:%d", pf_str, (void *)router, router->connections.size());
    auto sig_iter = router->connections.begin();
    while (sig_iter != router->connections.end()) {
        dlog("%s signal addr:%p  connections:%d", pf_str, sig_iter->first, sig_iter->second.size());
        for (sizet i = 0; i < sig_iter->second.size(); ++i) {
            dlog("%s connection addr: %p", pf_str, (void *)sig_iter->second[i].get());
        }
        ++sig_iter;
    }
}

void ss_disconnect_all(ss_router *router)
{
    router->connections.clear();
}
