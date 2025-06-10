#pragma once
#ifndef TIMER_HH
#define TIMER_HH
template<typename Timer>
class timer_set {
public:
    using time_point = typename Timer::time_point;
    using timer_list_t = std::list<Timer*>;
    using duration = typename Timer::duration;
    using timestamp_t = typename duration::rep;
    static constexpr timestamp_t max_timestamp = std::numeric_limits<timestamp_t>::max();
    static constexpr int timestamp_bits = std::numeric_limits<timestamp_t>::digits;//63
    static constexpr int n_buckets = timestamp_bits + 1;//64
    std::array<timer_list_t, n_buckets> _buckets;
    timestamp_t _last;
    timestamp_t _next;
    std::bitset<n_buckets> _non_empty_buckets;
    /// \brief 获取时间点对应的时间戳（计数）
    /// \param tp 时间点对象
    /// \return 时间点自纪元以来的计数值
    static timestamp_t get_timestamp(time_point tp) {
        return tp.time_since_epoch().count();
    }
    /// \brief 获取定时器的超时时间戳
    /// \param timer 定时器对象
    /// \return 定时器超时时间的时间戳
    static timestamp_t get_timestamp(Timer& timer) {
        return get_timestamp(timer.get_timeout());
    }
    /// \brief 根据时间戳计算对应的桶索引
    /// \param timestamp 要计算的定时器时间戳
    /// \return 对应的桶索引
    int get_index(timestamp_t timestamp) const {
        // std::cout << "timestamp: " << timestamp << ", _last: " << _last << std::endl;
        if (timestamp <= _last) {
            // std::cout << "-> Using fallback bucket: " << (n_buckets - 1) << std::endl;
            return n_buckets - 1;
        }
        auto index = bitsets::count_leading_zeros(timestamp ^ _last);
        // std::cout << "timestamp ^ _last: " << (timestamp ^ _last) << std::endl;
        // std::cout << " -> Calculated index: " << index << std::endl;
        assert(index < n_buckets - 1);
        return index;
    }
    /*这个需要手动推导一下*/
    /// \brief 获取定时器对应的桶索引
    /// \param timer 定时器对象
    /// \return 对应的桶索引
    int get_index(Timer& timer) const {
        return get_index(get_timestamp(timer));
    }
    /*
    一个timer有唯一的一个过期时间。
    */

    /// \brief 获取最后一个非空桶的索引
    /// \return 最后一个非空桶的索引
    int get_last_non_empty_bucket() const {
        return bitsets::get_last_set(_non_empty_buckets);
    }

public:
    /// \brief 构造函数初始化成员变量
    timer_set() : _last(0), _next(max_timestamp), _non_empty_buckets(0) {}

    ~timer_set() {
        // 清理所有定时器资源
        for (auto& list : _buckets) {
            while (!list.empty()) {
                auto* timer = list.front();
                list.pop_front();
                timer->cancel();
            }
        }
    }

    /// \brief 将定时器插入到对应的桶中
    /// \param timer 要插入的定时器对象
    /// \return true 如果插入后_next被更新为更小的值，否则false
    bool insert(Timer& timer) {
        auto timestamp = get_timestamp(timer);
        auto index = get_index(timestamp);
        auto& list = _buckets[index];
        list.push_back(&timer);
        timer.it = --list.end();//timer.it是timer在list中的迭代器(使用尾插法,所以end前一个位置就是最后一个元素前开后闭)
        _non_empty_buckets[index] = true;
        if (timestamp < _next) {
            _next = timestamp;
            return true;
        }
        return false;
    }
    /*
        next是边插入边维护的一个变量.表示下一次过期的时间.
    */

    /// \brief 从集合中移除定时器
    /// \param timer 要移除的定时器对象
    void remove(Timer& timer) {
        auto index = get_index(timer);
        auto& list = _buckets[index];
        list.erase(timer.it);//erase一个节点会造成内存泄漏吗? 不会:见 STL源码, 解析
        if (list.empty()) {
            _non_empty_buckets[index] = false;
        }
    }
    /**
     *
     * 这个地方像是RTOS优先级位图
     *
    */
    /// \brief 获取已到期的定时器列表
    /// \param now 当前时间点
    /// \return 包含所有已到期定时器的列表
    timer_list_t expire(time_point now) {
        timer_list_t exp;
        auto timestamp = get_timestamp(now);
        // std::cout << "Expire: now=" << now.time_since_epoch().count()
        //         << ", timestamp=" << timestamp
        //         << ", _last=" << _last << std::endl;

        if (timestamp < _last) {
            std::cerr << "ERROR: timestamp < _last, aborting!" << std::endl;
            abort();
        }
        auto index = get_index(timestamp);
        // std::cout << " -> Calculated index: " << index << std::endl;
        // 处理所有在当前 index 之前的非空桶
        // std::cout << "Scanning buckets up to index: " << index << std::endl;
        for (int i : bitsets::for_each_set(_non_empty_buckets, index + 1)) {
            // std::cout << "   Processing bucket[" << i << "] with " << _buckets[i].size() << " timers" << std::endl;
            exp.splice(exp.end(), _buckets[i]);
            _non_empty_buckets[i] = false;
        }
        _last = timestamp;
        _next = max_timestamp;
        auto& list = _buckets[index];
        // std::cout << "Processing current bucket[" << index << "] with " << list.size() << " timers" << std::endl;

        while (!list.empty()) {
            auto* timer = list.front();
            list.pop_front();

            auto timer_timeout = timer->get_timeout();
            auto timer_timestamp = get_timestamp(timer_timeout);

            // std::cout << "   Timer timeout: " << timer_timeout.time_since_epoch().count()
            //         << ", timestamp: " << timer_timestamp
            //         << ", is_expired: " << (timer->get_timeout() <= now ? "YES" : "NO") << std::endl;

            if (timer->get_timeout() <= now) {
                exp.push_back(timer);
            } else {
                insert(*timer); // 重新插入未过期的定时器
            }
        }

        _non_empty_buckets[index] = !list.empty();

        if (_next == max_timestamp && _non_empty_buckets.any()) {
            int last_bucket = get_last_non_empty_bucket();
            // std::cout << "Updating _next from non-empty bucket[" << last_bucket << "]" << std::endl;

            for (auto* timer : _buckets[last_bucket]) {
                auto ts = get_timestamp(*timer);
                _next = std::min(_next, ts);
                // std::cout << "   Min timestamp found: " << ts << std::endl;
            }
        }
        // std::cout << "Returning " << exp.size() << " expired timers." << std::endl;
        return exp;
    }

    time_point get_next_timeout() const {
        return time_point(duration(std::max(_last, _next)));
    }
    void clear() {
        for (auto& list : _buckets) {
            list.clear();
        }
        _non_empty_buckets.reset();
    }
    size_t size() const {
        size_t res = 0;
        for (const auto& list : _buckets) {
            res += list.size();
        }
        return res;
    }
    bool empty() const {
        return _non_empty_buckets.none();
    }
    time_point now() {
        return Timer::clock::now(); // now获取的就是当前时间.
    }
};

using steady_clock_type = std::chrono::steady_clock;


// timer 有默认参数,所以timer<>表示一个steady_clock_type
template <typename Clock = steady_clock_type>
class timer {
public:
    timer() = default;
    using time_point = typename Clock::time_point;
    using duration = typename Clock::duration;
    typedef Clock clock;
    using callback_t = std::function<void()>;
    using iterator = typename std::list<timer*>::iterator;
    iterator it; // 新增的迭代器成员
    iterator expired_it;  // 过期链表中的位置
    // boost::intrusive::list_member_hook<> _link;
    callback_t _callback;
    time_point _expiry; //到期时间点,每个timer都有一个到期时间点.
    std::optional<duration> _period;
    bool _armed = false;
    bool _queued = false;
    bool _expired = false;
    void readd_periodic();
    void arm_state(time_point until, std::optional<duration> period);
    timer(timer&& t) noexcept;
    explicit timer(callback_t&& callback);
    ~timer();
    // future<> expired();
    void set_callback(callback_t&& callback);
    void arm(time_point until, std::optional<duration> period = {});
    void rearm(time_point until, std::optional<duration> period = {});
    void rearm(duration delta) { rearm(Clock::now() + delta); }
    void arm(duration delta);
    void arm_periodic(duration delta);
    bool armed() const { return _armed; }
    bool cancel();
    time_point get_timeout();
};


class lowres_clock {
public:
    typedef int64_t rep;
    typedef std::ratio<1, 1000> period;
    typedef std::chrono::duration<rep, period> duration;
    typedef std::chrono::time_point<lowres_clock, duration> time_point;
    lowres_clock();
    static time_point now() {
        auto nr = _now.load(std::memory_order_relaxed);
        return time_point(duration(nr));
    }
private:
    static void update();
    static std::atomic<rep> _now [[gnu::aligned(64)]];
    struct timer_deleter {
        void operator()(void*) const;
    };
    timer<steady_clock_type> _timer;
    static constexpr std::chrono::milliseconds _granularity{10};
};

void enable_timer(steady_clock_type::time_point when);
bool queue_timer(timer<steady_clock_type>* tmr);
void add_timer(timer<steady_clock_type>* tmr);
void del_timer(timer<steady_clock_type>* tmr);
bool queue_timer(timer<lowres_clock>* tmr);
void add_timer(timer<lowres_clock>* tmr);
void del_timer(timer<lowres_clock>* tmr);
// extern std::atomic<lowres_clock::rep> lowres_clock::_now;
// extern constexpr std::chrono::milliseconds lowres_clock::_granularity;

#endif