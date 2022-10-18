#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <set>
#include <map>

#include "typedefs.h"
#include "logging.h"

struct ss_router;

using sizet = std::size_t;
using u32 = uint32_t;

struct ss_connection_base
{
    virtual ~ss_connection_base()
    {}
    virtual void* sig_addr() = 0;
    ss_router *router;
    u32 filter_key{0};
    std::string dbg_str;
    int dbg_invocation_cnt{0};
};

template<class... Args>
struct ss_signal;

// map simply to keep in visual order
inline std::map<ss_connection_base*, ss_connection_base*> g_debug_list;

template<class... Args>
struct ss_connection : public ss_connection_base
{
    ss_connection() {
        g_debug_list.emplace(this, this);
    }
    virtual ~ss_connection();
    void* sig_addr() override {return (void*)connected_signal;}

    virtual void call(Args...) = 0;
    ss_signal<Args...> *connected_signal;
};

template<class... Args>
struct ss_connection_func : public ss_connection<Args...>
{
    ss_connection_func(const std::function<void(Args...)> &func) : func_(func)
    {}

    void call(Args... args)
    {
        func_(args...);
        ++ss_connection_base::dbg_invocation_cnt;
    }

    std::function<void(Args...)> func_;
};

template<class T, class... Args>
struct ss_connection_mfunc : public ss_connection<Args...>
{
    typedef void (T::*mem_func_t)(Args...);

    ss_connection_mfunc(T *inst_, mem_func_t func_) : inst(inst_), func(func_)
    {}

    void call(Args... args)
    {
        (inst->*func)(args...);
        ++ss_connection_base::dbg_invocation_cnt;
    }

    T *inst;
    mem_func_t func;
};

template<class... Args>
void ss_call_slots(const ss_signal<Args...> &signal, u32 filter_key, Args&& ...args)
{
    // By using a temp copy instead of the originals, slot functions can disconnect from
    // signals - ie modify slot_connections without causing crashes - sort of - unless we are disconnecting
    // from this signal...
    std::vector<std::weak_ptr<ss_connection<Args...>>> tmp_copy(signal.connections);
    auto iter = tmp_copy.begin();
    while (iter != tmp_copy.end())
    {
        auto ptr = iter->lock();
        if (ptr && (ptr->filter_key == 0 || ptr->filter_key == filter_key))
            ptr->call(std::forward<Args>(args)...);
        ++iter;
    }
}

sizet ss_disconnect(ss_connection_base * connection);

template<class... Args>
struct ss_signal
{
    ss_signal()
    {}

    // Do not copy signals!
    ss_signal(const ss_signal<Args...> &) : connections()
    {}

    ~ss_signal()
    {
        while(connections.begin() != connections.end())
        {
            auto shptr = connections.back().lock();
            ss_disconnect(shptr.get());
            shptr->connected_signal = nullptr;
            connections.pop_back();
        }
    }

    void operator()(u32 sig_filter_key, Args... args)
    {
        ss_call_slots(*this, sig_filter_key, std::forward<Args>(args)...);
    }

    std::vector<std::weak_ptr<ss_connection<Args...>>> connections;
};

template<class... Args>
ss_connection<Args...>::~ss_connection()
{
    g_debug_list.erase(this);
    if (!connected_signal)
        return;
    
    auto iter = connected_signal->connections.begin();
    while (iter != connected_signal->connections.end())
    {
        auto sptr = iter->lock().get();
        if (!sptr || (sptr && (this == sptr)))
            iter = connected_signal->connections.erase(iter);
        else
            ++iter;
    }
}

#define emit_sig

using sptr_ss_conn_vector = std::vector<std::shared_ptr<ss_connection_base>>;

struct ss_router
{
    ss_router();
    ~ss_router();
    ss_router(const ss_router &copy);
    ss_router &operator=(const ss_router &rhs);
    std::map<void*,sptr_ss_conn_vector> connections;
};

void ss_remove_slot(ss_router *router, ss_connection_base *slot);

void ss_disconnect_all(ss_router *router);

template<class T, class... Args>
std::shared_ptr<ss_connection<Args...>> ss_connect(ss_signal<Args...> &sig, T *inst, void (T::*mf)(Args...))
{
    auto sptr = std::make_shared<ss_connection_mfunc<T, Args...>>(inst, mf);
    sptr->connected_signal = &sig;
    sptr->router = nullptr;
    sptr->filter_key = 0;
    sig.connections.emplace_back(std::static_pointer_cast<ss_connection<Args...>>(sptr));
    return sptr;
}

template<class LambdaType, class... Args>
std::shared_ptr<ss_connection<Args...>> ss_connect(ss_signal<Args...> &sig, const LambdaType &func)
{
    std::function<void(Args...)> f = func;
    auto sptr = std::make_shared<ss_connection_func<Args...>>(f);
    sptr->connected_signal = &sig;
    sptr->router = nullptr;
    sptr->filter_key = 0;
    sig.connections.emplace_back(std::static_pointer_cast<ss_connection<Args...>>(sptr));
    return sptr;
}

template<class LambdaType, class... Args>
std::shared_ptr<ss_connection<Args...>> ss_connect(ss_signal<Args...> &sig, u32 filter_key, const LambdaType &func)
{
    std::function<void(Args...)> f = func;
    auto sptr = std::make_shared<ss_connection_func<Args...>>(f);
    sptr->connected_signal = &sig;
    sptr->router = nullptr;
    sptr->filter_key = filter_key;
    sig.connections.emplace_back(std::static_pointer_cast<ss_connection<Args...>>(sptr));
    return sptr;
}

template<class T, class... Args>
std::shared_ptr<ss_connection<Args...>> ss_connect(ss_signal<Args...> &sig, u32 filter_key, T *inst, void (T::*mf)(Args...))
{
    auto sptr = std::make_shared<ss_connection_mfunc<T, Args...>>(inst, mf);
    sptr->connected_signal = &sig;
    sptr->router = nullptr;
    sptr->filter_key = filter_key;
    sig.connections.emplace_back(std::static_pointer_cast<ss_connection<Args...>>(sptr));
    return sptr;
}

template<class T, class... Args>
ss_connection<Args...> * ss_connect(ss_router *router, ss_signal<Args...> &sig, T *inst, void (T::*mf)(Args...))
{
    auto sptr = ss_connect(sig, inst, mf);
    sptr->router = router;
    router->connections[(void*)&sig].emplace_back(sptr);
    return sptr.get();
}

template<class LambdaType, class... Args>
ss_connection<Args...> * ss_connect(ss_router *router, ss_signal<Args...> &sig, const LambdaType &func)
{
    auto sptr = ss_connect(sig, func);
    sptr->router = router;
    router->connections[(void*)&sig].emplace_back(sptr);
    return sptr.get();
}

template<class LambdaType, class... Args>
ss_connection<Args...> *ss_connect(ss_router *router, ss_signal<Args...> &sig, u32 filter_key, const LambdaType &func)
{
    auto sptr = ss_connect(sig, filter_key, func);
    sptr->router = router;
    router->connections[(void*)&sig].emplace_back(sptr);
    return sptr.get();
}

template<class T, class... Args>
ss_connection<Args...> *ss_connect(ss_router *router, ss_signal<Args...> &sig, u32 filter_key, T *inst, void (T::*mf)(Args...))
{
    auto sptr = ss_connect(sig, filter_key, inst, mf);
    sptr->router = router;
    router->connections[(void*)&sig].emplace_back(sptr);
    return sptr.get();
}

template<class T>
void debug_log_connections(T *sig, const std::string &prefix="")
{
    const char *pf_str = prefix.c_str();
    dlog("%s signal addr:%p  connections:%d", pf_str, (void*)sig, sig->connections.size());
    for (int i = 0; i < sig->connections.size(); ++i)
    {
        auto ptr = sig->connections[i].lock();
        dlog("%s slot connection addr:%p fkey:%d", pf_str, (void*)(ptr.get()), ptr->filter_key);
    }
}

void debug_log_connections(ss_router * router, const std::string &prefix="");

template<class... Args>
sizet ss_disconnect(ss_router *router, ss_signal<Args...> &sig)
{
    sizet erase_cnt = 0;
    void* casted_sig = (void*)(&sig);
    auto fiter = router->connections.find(casted_sig);
    if (fiter != router->connections.end())
    {
        erase_cnt = fiter->second.size();
        router->connections.erase(fiter);
    }
    return erase_cnt;
}


template<class... Args>
sizet ss_disconnect(ss_router *router, ss_signal<Args...> &sig, u32 filter_key)
{
    sizet erase_cnt = 0;
    void* casted_sig = (void*)(&sig);
    auto fiter = router->connections.find(casted_sig);
    if (fiter != router->connections.end())
    {
        auto conn_iter = fiter->second.begin();
        while (conn_iter != fiter->second.end())
        {
            if ((*conn_iter)->filter_key == filter_key)
            {
                conn_iter = fiter->second.erase(conn_iter);
                ++erase_cnt;
            }
            else
            {
                ++conn_iter;
            }
        }

        if (fiter->second.empty())
        {
            router->connections.erase(fiter);
        }
    }
    return erase_cnt;
}

template<class... Args>
sizet ss_connection_count(ss_router *router, ss_signal<Args...> &sig)
{
    void* casted_sig = (void*)(&sig);
    auto fiter = router->connections.find(casted_sig);
    if (fiter != router->connections.end())
        return fiter->second.size();
    return 0;
}