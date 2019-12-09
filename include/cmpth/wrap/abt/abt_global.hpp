
#pragma once

#include <cmpth/wrap/abt/abt.hpp>

namespace cmpth {

template <typename P>
class abt_global
{
    using pool_type = typename P::pool_type;
    using xstream_type = typename P::xstream_type;

    using constants_type = typename P::constants_type;

public:
    explicit abt_global(const int argc, char** const argv) {
        const char* const num_wks_str = std::getenv("CMPTH_NUM_WORKERS");
        int num_wks = num_wks_str ? std::atoi(num_wks_str) : 1;
        this->num_xstreams_ = num_wks;

        abt_error::check_error(ABT_init(argc, argv));

        const auto enable_abt_private_pool = constants_type::enable_abt_private_pool;
        if (enable_abt_private_pool) {
            this->pools_ = fdn::make_unique<pool_type []>(num_wks);
            for (int i = 0; i < num_wks; ++i) {
                this->pools_[i] =
                    pool_type::create_basic(ABT_POOL_FIFO, ABT_POOL_ACCESS_PRIV, ABT_TRUE);
            }
        }
        else {
            this->pools_ = fdn::make_unique<pool_type []>(1);
            this->pools_[0] =
                pool_type::create_basic(ABT_POOL_FIFO, ABT_POOL_ACCESS_MPMC, ABT_TRUE);
        }

        auto self_xstream = xstream_type::self();
        {
            auto pool = this->pools_[0].get();
            self_xstream.set_main_sched_basic(ABT_SCHED_RANDWS, 1, &pool);
        }
        this->xstreams_ = fdn::make_unique<xstream_type []>(num_wks);
        this->xstreams_[0] = fdn::move(self_xstream);

        for (int i = 1; i < num_wks; ++i) {
            const auto pool_idx = enable_abt_private_pool ? i : 0;
            auto pool = this->pools_[pool_idx].get();
            auto xstream =
                xstream_type::create_basic(
                    ABT_SCHED_RANDWS, 1, &pool, ABT_SCHED_CONFIG_NULL);
            xstream.start();

            this->xstreams_[i] = fdn::move(xstream);
        }
    }

    abt_global(const abt_global&) = delete;
    abt_global& operator = (const abt_global&) = delete;

    ~abt_global() /*noexcept*/ {
        for (int i = 1; i < this->num_xstreams_; ++i) {
            this->xstreams_[i].join();
        }
        this->xstreams_[0].release();

        ABT_finalize();
    }

    ABT_pool get_pool(const fdn::size_t rank) const noexcept {
        const auto pool_idx = constants_type::enable_abt_private_pool ? rank : 0;
        return this->pools_[pool_idx].get();
    }

    static void init(const int argc, char** const argv) {
        CMPTH_P_ASSERT(P, gl_ == nullptr);
        gl_ = new abt_global{argc, argv};
    }
    static void finalize() noexcept{
        CMPTH_P_ASSERT(P, gl_ != nullptr);
        delete gl_;
        gl_ = nullptr;
    }
    static abt_global& get_instance() noexcept {
        CMPTH_P_ASSERT(P, gl_ != nullptr);
        return *gl_;
    }

private:
    int num_xstreams_ = 0;
    fdn::unique_ptr<pool_type []> pools_;
    fdn::unique_ptr<xstream_type []> xstreams_;

    static abt_global* gl_;
};

template <typename P>
abt_global<P>* abt_global<P>::gl_ = nullptr;

} // namespace cmpth

