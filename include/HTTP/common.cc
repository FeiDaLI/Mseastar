#include "common.hh"

namespace httpd {

operation_type str2type(const sstring& type) {
    if (type == "DELETE") {
        return DELETE;
    }
    if (type == "POST") {
        return POST;
    }
    if (type == "PUT") {
        return PUT;
    }
    return GET;
}

}
