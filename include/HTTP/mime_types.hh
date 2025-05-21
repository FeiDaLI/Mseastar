#ifndef HTTP_MIME_TYPES_HH
#define HTTP_MIME_TYPES_HH

namespace httpd {
    namespace mime_types {
        const char* extension_to_type(const std::string& extension);
    }
}

namespace httpd {
    namespace mime_types {
    struct mapping {
        const char* extension;
        const char* mime_type;
    } mappings[] = {
        { "gif", "image/gif" },
        { "htm", "text/html" },
        { "css", "text/css" },
        { "js", "text/javascript" },
        { "html", "text/html" },
        { "jpg", "image/jpeg" },
        { "png", "image/png" },
        { "txt", "text/plain" },
        { "ico", "image/x-icon" },
        { "bin", "application/octet-stream" },
        { "proto", "application/vnd.google.protobuf; proto=io.prometheus.client.MetricFamily; encoding=delimited"}, 
    };

const char* extension_to_type(const std::string& extension){
    for (mapping m : mappings) {
        if (extension == m.extension) {
            return m.mime_type;
        }
    }
    return "text/plain";
    }
  }
}



#endif // HTTP_MIME_TYPES_HH
