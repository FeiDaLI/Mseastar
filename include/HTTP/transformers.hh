#ifndef TRANSFORMERS_HH_
#define TRANSFORMERS_HH_
#include "handlers.hh"
#include "file_handler.hh"
#include <boost/algorithm/string/replace.hpp>
#include <string_view>

namespace httpd {

class content_replace : public file_transformer {
public:
    virtual void transform(std::string& content, const request& req, const std::string& extension) override;
    explicit content_replace(const std::string& extension = "")
            : extension(extension) {}
private:
    std::string extension;
};

}


namespace httpd {
    using namespace std;

void content_replace::transform(std::string& content, const request& req,
        const std::string& extension) {
    std::string host = req.get_header("Host");
    if (host == "" || (this->extension != "" && extension != this->extension)) {
        return;
    }
    boost::replace_all(content, "{{Host}}", host);
    boost::replace_all(content, "{{Protocol}}", req.get_protocol_name());
}
}


#endif /* TRANSFORMERS_HH_ */
