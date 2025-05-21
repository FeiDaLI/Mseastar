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
 * Copyright 2015 Cloudius Systems
 */

// This file was modified from boost http example
//
// reply.hpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <unordered_map>
#include "mime_types.hh"

namespace httpd {
/**
 * A reply to be sent to a client.
 */
struct reply {
    /**
     * The status of the reply.
     */
    enum class status_type {
        ok = 200, //!< ok
        created = 201, //!< created
        accepted = 202, //!< accepted
        no_content = 204, //!< no_content
        multiple_choices = 300, //!< multiple_choices
        moved_permanently = 301, //!< moved_permanently
        moved_temporarily = 302, //!< moved_temporarily
        not_modified = 304, //!< not_modified
        bad_request = 400, //!< bad_request
        unauthorized = 401, //!< unauthorized
        forbidden = 403, //!< forbidden
        not_found = 404, //!< not_found
        internal_server_error = 500, //!< internal_server_error
        not_implemented = 501, //!< not_implemented
        bad_gateway = 502, //!< bad_gateway
        service_unavailable = 503  //!< service_unavailable
    } _status;

    /**
     * The headers to be included in the reply.
     */
    std::unordered_map<std::string, std::string> _headers;

    std::string _version;
    /**
     * The content to be sent in the reply.
     */
    std::string _content;

    std::string _response_line;
    reply()
            : _status(status_type::ok) {
    }

    reply& add_header(const std::string& h, const std::string& value) {
        _headers[h] = value;
        return *this;
    }

    reply& set_version(const std::string& version) {
        _version = version;
        return *this;
    }
    reply& set_status(status_type status, const std::string& content = "") {
        _status = status;
        if (content != "") {
            _content = content;
        }
        return *this;
    }
    /**
     * Set the content type mime type.
     * Used when the mime type is known.
     * For most cases, use the set_content_type
     */
    reply& set_mime_type(const std::string& mime) {
        _headers["Content-Type"] = mime;
        return *this;
    }
    /**
     * Set the content type mime type according to the file extension
     * that would have been used if it was a file: e.g. html, txt, json etc'
     */
    reply& set_content_type(const std::string& content_type = "html") {
        set_mime_type(httpd::mime_types::extension_to_type(content_type));
        return *this;
    }
    reply& done(const std::string& content_type) {
        return set_content_type(content_type).done();
    }
    /**
     * Done should be called before using the reply.
     * It would set the response line
     */
    reply& done() {
        _response_line = response_line();
        return *this;
    }
    std::string response_line();
};

} // namespace httpd
namespace httpd {
namespace status_strings {
    const std::string ok = " 200 OK\r\n";
    const std::string created = " 201 Created\r\n";
    const std::string accepted = " 202 Accepted\r\n";
    const std::string no_content = " 204 No Content\r\n";
    const std::string multiple_choices = " 300 Multiple Choices\r\n";
    const std::string moved_permanently = " 301 Moved Permanently\r\n"; 
    const std::string moved_temporarily = " 302 Moved Temporarily\r\n";
    const std::string not_modified = " 304 Not Modified\r\n";
    const std::string bad_request = " 400 Bad Request\r\n";
    const std::string unauthorized = " 401 Unauthorized\r\n";
    const std::string forbidden = " 403 Forbidden\r\n";
    const std::string not_found = " 404 Not Found\r\n";
    const std::string internal_server_error = " 500 Internal Server Error\r\n";
    const std::string not_implemented = " 501 Not Implemented\r\n";
    const std::string bad_gateway = " 502 Bad Gateway\r\n";
    const std::string service_unavailable = " 503 Service Unavailable\r\n";
static const std::string& to_string(reply::status_type status) {
    switch (status) {
    case reply::status_type::ok:
        return ok;
    case reply::status_type::created:
        return created;
    case reply::status_type::accepted:
        return accepted;
    case reply::status_type::no_content:
        return no_content;
    case reply::status_type::multiple_choices:
        return multiple_choices;
    case reply::status_type::moved_permanently:
        return moved_permanently;
    case reply::status_type::moved_temporarily:
        return moved_temporarily;
    case reply::status_type::not_modified:
        return not_modified;
    case reply::status_type::bad_request:
        return bad_request;
    case reply::status_type::unauthorized:
        return unauthorized;
    case reply::status_type::forbidden:
        return forbidden;
    case reply::status_type::not_found:
        return not_found;
    case reply::status_type::internal_server_error:
        return internal_server_error;
    case reply::status_type::not_implemented:
        return not_implemented;
    case reply::status_type::bad_gateway:
        return bad_gateway;
    case reply::status_type::service_unavailable:
        return service_unavailable;
    default:
        return internal_server_error;
    }
}
} // namespace status_strings

std::string reply::response_line() {
    return "HTTP/" + _version + status_strings::to_string(_status);
}

} // namespace server
