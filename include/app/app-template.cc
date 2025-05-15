// #include "app-template.hh"
// // #include "../timer/timer_all.hh"
// #include <boost/program_options.hpp>
// #include <boost/program_options.hpp>
// #include <boost/make_shared.hpp>
// #include <fstream>
// #include <cstdlib>

// namespace bpo = boost::program_options;

// app_template::app_template(app_template::config cfg)
//     : _cfg(std::move(cfg))
//     , _opts(_cfg.name + " options") {
//         _opts.add_options()
//                 ("help,h", "show help message")
//                 ;
//         _opts.add(reactor::get_options_description());
//         // _opts.add(seastar::metrics::get_options_description());
//         // _opts.add(smp::get_options_description());
//         // _opts.add(scollectd::get_options_description());
// }

// boost::program_options::options_description_easy_init
// app_template::add_options() {
//     return _opts.add_options();
// }

// void
// app_template::add_positional_options(std::initializer_list<positional_option> options) {
//     for (auto&& o : options) {
//         _opts.add(boost::make_shared<bpo::option_description>(o.name, o.value_semantic, o.help));
//         _pos_opts.add(o.name, o.max_count);
//     }
// }


// bpo::variables_map&
// app_template::configuration() {
//     return *_configuration;
// }

// int
// app_template::run(int ac, char ** av, std::function<future<int> ()>&& func) {
//     return run_deprecated(ac, av, [func = std::move(func)] () mutable {
//         auto func_done = make_lw_shared<promise<>>();
//         engine().at_exit([func_done] { return func_done->get_future(); });
//         futurize_apply(func).finally([func_done] {
//             func_done->set_value();
//         }).then([] (int exit_code) {
//             return engine().exit(exit_code);
//         }).or_terminate();
//     });
// }

// int app_template::run(int ac, char ** av, std::function<future<> ()>&& func) {
//     return run(ac, av, [func = std::move(func)] {
//         return func().then([] () {
//             return 0;
//         });
//     });
// }

// int app_template::run_deprecated(int ac, char ** av, std::function<void ()>&& func) {
//     bpo::variables_map configuration;
//     try {
//         bpo::store(bpo::command_line_parser(ac, av)
//                     .options(_opts)
//                     .positional(_pos_opts)
//                     .run()
//             , configuration);
//         auto home = std::getenv("HOME");
//         if (home) {
//             std::ifstream ifs(std::string(home) + "/.config/seastar/seastar.conf");
//             if (ifs) {
//                 bpo::store(bpo::parse_config_file(ifs, _opts), configuration);
//             }
//             std::ifstream ifs_io(std::string(home) + "/.config/seastar/io.conf");
//             if (ifs_io) {
//                 bpo::store(bpo::parse_config_file(ifs_io, _opts), configuration);
//             }
//         }
//     } catch (bpo::error& e) {
//         std::cout<<"error: "<<e.what()<<"\n";
//         return 2;
//     }
//     if (configuration.count("help")) {
//         std::cout << _opts << "\n";
//         return 1;
//     }

//     bpo::notify(configuration);
//     configuration.emplace("argv0", boost::program_options::variable_value(std::string(av[0]), false));
//     smp::configure(configuration);
//     _configuration = {std::move(configuration)};
//     engine().when_started().then([this] {
//         //     metrics::configure(this->configuration()).then([this] {
//         //     // set scollectd use the metrics configuration, so the later
//         //     // need to be set first
//         //     scollectd::configure( this->configuration());
//         // });
//         std::cout<<"配置信息"<<std::endl;
//     }).then(
//         std::move(func)
//     ).then_wrapped([] (auto&& f) {
//         try {
//             f.get();
//         } catch (std::exception& ex) {
//             std::cout << "program failed with uncaught exception: " << ex.what() << "\n";
//             engine().exit(1);
//         }
//     });
//     auto exit_code = engine().run();
//     smp::cleanup();
//     return exit_code;
// }
