/*
 * This file is open source software, licensed to you under the terms
 * of the Apache License, Version 2.0 (the "License").  See the NOTICE file
 * distributed with this work for additional information regarding copyright
 * ownership.  You may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/*
 * Copyright (C) 2014 Cloudius Systems, Ltd.
 */
#ifndef _APP_TEMPLATE_HH
#define _APP_TEMPLATE_HH

#include <boost/program_options.hpp>
#include <boost/optional.hpp>
#include <functional>
#include "../future/future_all10.hh"
#include <string>
#include <boost/program_options.hpp>
#include <boost/program_options.hpp>
#include <boost/make_shared.hpp>
#include <fstream>
#include <cstdlib>

class app_template {
public:
    struct config {
        std::string name;
    };
private:
    config _cfg;
    boost::program_options::options_description _opts;
    boost::program_options::positional_options_description _pos_opts;
    boost::optional<boost::program_options::variables_map> _configuration;
public:
    struct positional_option {
        const char* name;
        const boost::program_options::value_semantic* value_semantic;
        const char* help;
        int max_count;
    };
public:
    explicit app_template(config cfg = config{"App"});
    boost::program_options::options_description_easy_init add_options();
    void add_positional_options(std::initializer_list<positional_option> options);
    boost::program_options::variables_map& configuration();
    int run_deprecated(int ac, char ** av, std::function<void ()>&& func);
    // Runs given function and terminates the application when the future it
    // returns resolves. The value with which the future resolves will be
    // returned by this function.
    int run(int ac, char ** av, std::function<future<int> ()>&& func);
    // Like run_sync() which takes std::function<future<int>()>, but returns
    // with exit code 0 when the future returned by func resolves
    // successfully.
    int run(int ac, char ** av, std::function<future<> ()>&& func);
};



namespace bpo = boost::program_options;

app_template::app_template(app_template::config cfg) //这里的cfg就是一个string
    : _cfg(std::move(cfg))
    , _opts(_cfg.name + " options") {
        _opts.add_options()
                ("help,h", "show help message")
                ;
        _opts.add(reactor::get_options_description());
        // _opts.add(seastar::metrics::get_options_description());
        _opts.add(smp::get_options_description());
        // _opts.add(scollectd::get_options_description());
}

boost::program_options::options_description_easy_init
app_template::add_options() {
    return _opts.add_options();
}

void
app_template::add_positional_options(std::initializer_list<positional_option> options) {
    for (auto&& o : options) {
        _opts.add(boost::make_shared<bpo::option_description>(o.name, o.value_semantic, o.help));
        _pos_opts.add(o.name, o.max_count);
    }
}


bpo::variables_map&
app_template::configuration() {
    return *_configuration;
}

int
app_template::run(int ac, char ** av, std::function<future<int> ()>&& func) {
    return run_deprecated(ac, av, [func = std::move(func)] () mutable {
        auto func_done = make_lw_shared<promise<>>();
        engine().at_exit([func_done] { return func_done->get_future(); });
        futurize_apply(func).finally([func_done] {
            func_done->set_value();
        }).then([] (int exit_code) {
            return engine().exit(exit_code);
        }).or_terminate();
    });
}

int app_template::run(int ac, char ** av, std::function<future<> ()>&& func) {
    std::cout << "Entering app_template::run with future function\n";
    return run(ac, av, [func = std::move(func)] {
        return func().then([] () {
            std::cout << "Completing future function in app_template::run\n";
            return 0;
        });
    });
}

int app_template::run_deprecated(int ac, char ** av, std::function<void ()>&& func) {
    std::cout << "Entering app_template::run_deprecated\n";
    bpo::variables_map configuration;
    try {
        std::cout << "Parsing command line options\n";
        bpo::store(bpo::command_line_parser(ac, av)
                    .options(_opts)
                    .positional(_pos_opts)
                    .run()
            , configuration);
        std::cout<<"parse end"<<std::endl;
        auto home = std::getenv("HOME");
        if (home) {
            std::string config_path = std::string(home) + "/.config/seastar/seastar.conf";
            std::ifstream ifs(config_path);
            if (ifs) {
                std::cout << "Loading configuration from: " << config_path << "\n";
                bpo::store(bpo::parse_config_file(ifs, _opts), configuration);
            }
            std::string io_config_path = std::string(home) + "/.config/seastar/io.conf";
            std::ifstream ifs_io(io_config_path);
            if (ifs_io) {
                std::cout << "Loading IO configuration from: " << io_config_path << "\n";
                bpo::store(bpo::parse_config_file(ifs_io, _opts), configuration);
            }
        }
    } catch (bpo::error& e) {
        std::cout << "Error parsing options: " << e.what() << "\n";
        return 2;
    }

    if (configuration.count("help")) {
        std::cout << "Help requested. Printing options:\n" << _opts << "\n";
        return 1;
    }
    std::cout << "Notifying configuration changes\n";
    bpo::notify(configuration);//把命令行或者文件中的配置保存到variable_map中.
    std::cout << "Configuring SMP\n";
    configuration.emplace("argv0", boost::program_options::variable_value(std::string(av[0]), false));
    std::cout << "Configuring SMP 2\n";
    smp::configure(configuration);
    std::cout<<"smp configure end\n"<<std::endl;
    _configuration = {std::move(configuration)};
    std::cout << "Starting engine\n";
    std::cout<<"app的engine id"<<engine()._id<<std::endl;
    engine().when_started().then([this] {
        std::cout << "Engine started. Configuring metrics and scollectd\n";
        // metrics::configure(this->configuration()).then([this] {
        //     scollectd::configure(this->configuration());
        // });
        std::cout << "Configuration information loaded\n";
    },"when started config").then(
        std::move(func),"main中的func"
    ).then_wrapped([] (auto&& f) {
        try {
            f.get();
            std::cout << "Future completed successfully\n";
        } catch (std::exception& ex) {
            std::cout << "Program failed with uncaught exception: " << ex.what() << "\n";
            engine().exit(1);
        }
    });
    std::cout << "Running engine\n";
    auto exit_code = engine().run();
    // std::cout << "Engine exited with code: " << exit_code << "\n";
    // std::cout << "Cleaning up SMP\n";
    smp::cleanup();
    return exit_code;
}
#endif
