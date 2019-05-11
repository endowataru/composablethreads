
#pragma once

#include <cmpth/wrap/mth/mth.hpp>
#include <cmpth/ult_tag.hpp>
#include <cmpth/exec/basic_for_loop.hpp>
#include <cmpth/smth/default_smth.hpp> // TODO

#define CMPTH_ENABLE_MTH_WORKER_CACHE

namespace cmpth {

struct mth_error : std::exception { };

struct mth_base_policy
{
    static const bool is_debug =
        #ifdef CMPTH_DEBUG
        true;
        #else
        false;
        #endif
    
    using assert_policy_type = assert_policy<is_debug>;
    using log_policy_type = log_policy<is_debug>;
};

class mth_thread;
class mth_mutex;
class mth_cond_var;
class mth_uncond_var;
class mth_barrier;

template <typename P>
class mth_thread_specific;

struct mth_for_loop_policy
    : mth_base_policy
{
    using thread_type = mth_thread;
};

struct mth_itf
    : basic_for_loop<mth_for_loop_policy>
{
    struct initializer { };
    // TODO: Call myth_init() / myth_fini() ?
    
    using thread = mth_thread;
    
    using spinlock = default_smth_spinlock; // TODO
    
    using mutex = mth_mutex;
    using condition_variable = mth_cond_var;
    using uncond_variable = mth_uncond_var;
    
    using barrier = mth_barrier;
    
    template <typename P>
    using thread_specific = mth_thread_specific<P>;
    
    template <typename Mutex>
    using unique_lock = std::unique_lock<Mutex>;
    
    using unique_mutex_lock = fdn::unique_lock<mutex>;
    
    template <typename T>
    using atomic = std::atomic<T>;
    
    struct this_thread {
        static myth_thread_t native_handle() noexcept {
            return myth_self();
        }
    };
    
    #ifdef CMPTH_ENABLE_MTH_WORKER_CACHE
    static fdn::size_t get_worker_num() noexcept
    {
        static thread_local fdn::size_t wk_num_ = 0;
        auto wk_num = wk_num_;
        if (CMPTH_UNLIKELY(wk_num == 0)) {
            wk_num = static_cast<fdn::size_t>(myth_get_worker_num())+1;
            wk_num_ = wk_num;
        }
        return wk_num-1;
    }
    #else
    static fdn::size_t get_worker_num() noexcept
    {
        return myth_get_worker_num();
    }
    #endif
    static fdn::size_t get_num_workers() noexcept
    {
        return myth_get_num_workers();
    }
    
    using assert_policy = mth_base_policy::assert_policy_type;
    using log_policy = mth_base_policy::log_policy_type;
};


template <>
struct get_ult_itf_type<ult_tag_t::MTH>
    : fdn::type_identity<mth_itf> { };

} // namespace cmpth
