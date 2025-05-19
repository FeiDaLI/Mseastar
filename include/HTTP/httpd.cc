
#include "../include/future/future_all12.hh"
#include "core/app-template.hh"
#include "core/circular_buffer.hh"
#include "core/distributed.hh"
#include "core/queue.hh"
#include "core/future-util.hh"
#include "core/metrics.hh"
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <queue>
#include <bitset>
#include <limits>
#include <cctype>
#include <vector>
#include "httpd.hh"

using namespace std::chrono_literals;

namespace httpd {
http_stats::http_stats(http_server& server, const sstring& name)
 {
    namespace sm = seastar::metrics;
    std::vector<sm::label_instance> labels;

    labels.push_back(sm::label_instance("service", name));
    _metric_groups.add_group("httpd", {
            sm::make_derive("connections_total", [&server] { return server.total_connections(); }, sm::description("The total number of connections opened"), labels),
            sm::make_gauge("connections_current", [&server] { return server.current_connections(); }, sm::description("The current number of open  connections"), labels),
            sm::make_derive("read_errors", [&server] { return server.read_errors(); }, sm::description("The total number of errors while reading http requests"), labels),
            sm::make_derive("reply_errors", [&server] { return server.reply_errors(); }, sm::description("The total number of errors while replying to http"), labels),
            sm::make_derive("requests_served", [&server] { return server.requests_served(); }, sm::description("The total number of http requests served"), labels)
    });
}

sstring http_server_control::generate_server_name() {
    static thread_local uint16_t idgen;
    return seastar::format("http-{}", idgen++);
}
}
