#ifndef API_DOCS_HH_
#define API_DOCS_HH_

#include "json/json_elements.hh"
#include "json/formatter.hh"
#include "routes.hh"
#include "transformers.hh"
#include <string>
#include <boost/json.hpp>

namespace bj = boost::json;
namespace httpd {
   struct api_doc {
        std::string path;
        std::string description;
        bj::object to_json() const {
            return bj::object{
                {"path", path},
                {"description", description}
            };
        }
    };
    struct api_docs {
        std::string apiVersion = "0.0.1";
        std::string swaggerVersion = "1.2";
        std::vector<api_doc> apis;
        bj::object to_json() const {
            bj::array j_apis;
            for (const auto& api : apis) {
                j_apis.push_back(api.to_json());
            }
            return bj::object{
                {"apiVersion", apiVersion},
                {"swaggerVersion", swaggerVersion},
                {"apis", j_apis}
            };
        }
    };

class api_registry : public handler_base {
    std::string _base_path;
    std::string _file_directory;
    api_docs _docs;
    routes& _routes;

public:
    api_registry(routes& routes, const std::string& file_directory,
            const std::string& base_path)
            : _base_path(base_path), _file_directory(file_directory), _routes(
                    routes) {
        _routes.put(GET, _base_path, this);
    }
    future<std::unique_ptr<reply>> handle(const std::string& path,
            std::unique_ptr<request> req, std::unique_ptr<reply> rep) override {
        rep->_content = json::formatter::to_json(_docs);
        rep->done("json");
        return make_ready_future<std::unique_ptr<reply>>(std::move(rep));
    }

    void reg(const std::string& api, const std::string& description,
            const std::string& alternative_path = "") {
        api_doc doc;
        doc.description = description;
        doc.path = "/" + api;
        _docs.apis.push(doc);
        std::string path =
                (alternative_path == "") ?
                        _file_directory + api + ".json" : alternative_path;
        file_handler* index = new file_handler(path,
                new content_replace("json"));
        _routes.put(GET, _base_path + "/" + api, index);
    }
};

class api_registry_builder {
    std::string _file_directory;
    std::string _base_path;

public:
    static const std::string DEFAULT_DIR;
    static const std::string DEFAULT_PATH;

    api_registry_builder(const std::string& file_directory = DEFAULT_DIR,
            const std::string& base_path = DEFAULT_PATH)
            : _file_directory(file_directory), _base_path(base_path) {
    }

    void set_api_doc(routes& r) {
        new api_registry(r, _file_directory, _base_path);
    }

    void register_function(routes& r, const std::string& api,
            const std::string& description, const std::string& alternative_path = "") {
        auto h = r.get_exact_match(GET, _base_path);
        if (h) {
            // if a handler is found, it was added there by the api_registry_builder
            // with the set_api_doc method, so we know it's the type
            static_cast<api_registry*>(h)->reg(api, description, alternative_path);
        };
    }
};

}


#include "api_docs.hh"
#include "handlers.hh"
#include "transformers.hh"
using namespace std;
namespace httpd {
    const std::string api_registry_builder::DEFAULT_PATH = "/api-doc";
    const std::string api_registry_builder::DEFAULT_DIR = ".";
}


#endif /* API_DOCS_HH_ */
