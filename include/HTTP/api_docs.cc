#include "api_docs.hh"
#include "handlers.hh"
#include "json/formatter.hh"
#include "transformers.hh"
using namespace std;
namespace httpd {
    const std::string api_registry_builder::DEFAULT_PATH = "/api-doc";
    const std::string api_registry_builder::DEFAULT_DIR = ".";
}
