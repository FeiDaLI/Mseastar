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

#ifndef HTTP_FILE_HANDLER_HH_
#define HTTP_FILE_HANDLER_HH_

#include "handlers.hh"
#include <algorithm>
#include <iostream>
#include "../include/future/future_all12.hh"
#include "../include/app/app-template.hh"
#include "exception.hh"

namespace httpd {
/**
 * This is a base class for file transformer.
 *
 * File transformer adds the ability to modify a file content before returning
 * the results.
 *
 * The transformer decides according to the file extension if transforming is
 * needed.
 */
class file_transformer {
public:
    /**
     * Any file transformer should implement this method.
     * @param content the content to transform
     * @param req the request
     * @param extension the file extension originating the content
     */
    virtual void transform(std::string& content, const request& req,
            const std::string& extension) = 0;

    virtual ~file_transformer() = default;
};

/**
 * A base class for handlers that interact with files.
 * directory and file handlers both share some common logic
 * with regards to file handling.
 * they both needs to read a file from the disk, optionally transform it,
 * and return the result or page not found on error
 */
class file_interaction_handler : public handler_base {
public:
    file_interaction_handler(file_transformer* p = nullptr)
            : transformer(p) {}
    ~file_interaction_handler();

    /**
     * Allows setting a transformer to be used with the files returned.
     * @param t the file transformer to use
     * @return this
     */
    file_interaction_handler* set_transformer(file_transformer* t) {
        transformer = t;
        return this;
    }
    /**
     * if the url ends without a slash redirect
     * @param req the request
     * @param rep the reply
     * @return true on redirect
     */
    bool redirect_if_needed(const request& req, reply& rep) const;
    /**
     * A helper method that returns the file extension.
     * @param file the file to check
     * @return the file extension
     */
    static std::string get_extension(const std::string& file);
protected:

    /**
     * read a file from the disk and return it in the replay.
     * @param file the full path to a file on the disk
     * @param req the reuest
     * @param rep the reply
     */
    future<std::unique_ptr<reply> > read(const std::string& file,
            std::unique_ptr<request> req, std::unique_ptr<reply> rep);
    file_transformer* transformer;
};
/**
 * The directory handler get a disk path in the
 * constructor.
 * and expect a path parameter in the handle method.
 * it would concatenate the two and return the file
 * e.g. if the path is /usr/mgmt/public in the path
 * parameter is index.html
 * handle will return the content of /usr/mgmt/public/index.html
 */
class directory_handler : public file_interaction_handler {
public:
    /**
     * The directory handler map a base path and a path parameter to a file
     * @param doc_root the root directory to search the file from.
     * For example if the root is '/usr/mgmt/public' and the path parameter
     * will be '/css/style.css' the file wil be /usr/mgmt/public/css/style.css'
     */
    explicit directory_handler(const std::string& doc_root,
            file_transformer* transformer = nullptr);
    future<std::unique_ptr<reply>> handle(const std::string& path,
            std::unique_ptr<request> req, std::unique_ptr<reply> rep) override;
private:
    std::string doc_root;
};
/**
 * The file handler get a path to a file on the disk
 * in the constructor.
 * it will always return the content of the file.
 */
class file_handler : public file_interaction_handler {
public:
    /**
     * The file handler map a file to a url
     * @param file the full path to the file on the disk
     */
    explicit file_handler(const std::string& file, file_transformer* transformer =
            nullptr, bool force_path = true)
            : file_interaction_handler(transformer), file(file), force_path(
                    force_path) {
    }
    future<std::unique_ptr<reply>> handle(const std::string& path,
            std::unique_ptr<request> req, std::unique_ptr<reply> rep) override;
private:
    std::string file;
    bool force_path;
};

}

namespace httpd {

directory_handler::directory_handler(const std::string& doc_root,file_transformer* transformer): 
    file_interaction_handler(transformer),doc_root(doc_root){}


future<std::unique_ptr<reply>> directory_handler::handle(const std::string& path,
        std::unique_ptr<request> req, std::unique_ptr<reply> rep) {
    std::string full_path = doc_root + req->param["path"];
    auto h = this;
    return engine().file_type(full_path).then(
            [h, full_path, req = std::move(req), rep = std::move(rep)](auto val) mutable {
                if (val) {
                    if (val.value() == directory_entry_type::directory) {
                        if (h->redirect_if_needed(*req.get(), *rep.get())) {
                            return make_ready_future<std::unique_ptr<reply>>(std::move(rep));
                        }
                        full_path += "/index.html";
                    }
                    return h->read(full_path, std::move(req), std::move(rep));
                }
                rep->set_status(reply::status_type::not_found).done();
                return make_ready_future<std::unique_ptr<reply>>(std::move(rep));

    });
}

file_interaction_handler::~file_interaction_handler() {
    delete transformer;
}

std::string file_interaction_handler::get_extension(const std::string& file) {
    size_t last_slash_pos = file.find_last_of('/');
    size_t last_dot_pos = file.find_last_of('.');
    std::string extension;
    if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos) {
        extension = file.substr(last_dot_pos + 1);
    }
    return extension;
}

struct reader {
private:
public:
    reader(file f, std::unique_ptr<reply> rep)
            : is(make_file_input_stream(std::move(f)))
            , _rep(std::move(rep)) {
    }
    input_stream<char> is;
    std::unique_ptr<reply> _rep;

    // for input_stream::consume():
    using unconsumed_remainder = std::optional<temporary_buffer<char>>;
    future<unconsumed_remainder> operator()(temporary_buffer<char> data) {
        if (data.empty()) {
            _rep->done();
            return make_ready_future<unconsumed_remainder>(std::move(data));
        } else {
            _rep->_content.append(data.get(), data.size());
            return make_ready_future<unconsumed_remainder>();
        }
    }
};

future<std::unique_ptr<reply>> file_interaction_handler::read(
        const std::string& file_name, std::unique_ptr<request> req,
        std::unique_ptr<reply> rep) {
    std::string extension = get_extension(file_name);
    rep->set_content_type(extension);
    return open_file_dma(file_name, open_flags::ro).then(
            [rep = std::move(rep), extension, this, req = std::move(req)](file f) mutable {
                std::shared_ptr<reader> r = std::make_shared<reader>(std::move(f), std::move(rep));

                return r->is.consume(*r).then([r, extension, this, req = std::move(req)]() {
                            if (transformer != nullptr) {
                                transformer->transform(r->_rep->_content, *req, extension);
                            }
                            r->_rep->done();
                            return make_ready_future<std::unique_ptr<reply>>(std::move(r->_rep));
                        });
            });
}

bool file_interaction_handler::redirect_if_needed(const request& req,
        reply& rep) const {
    if (req._url.length() == 0 || req._url.back() != '/') {
        rep.set_status(reply::status_type::moved_permanently);
        rep._headers["Location"] = req.get_url() + "/";
        rep.done();
        return true;
    }
    return false;
}

future<std::unique_ptr<reply>> file_handler::handle(const std::string& path,
        std::unique_ptr<request> req, std::unique_ptr<reply> rep) {
    if (force_path && redirect_if_needed(*req.get(), *rep.get())) {
        return make_ready_future<std::unique_ptr<reply>>(std::move(rep));
    }
    return read(file, std::move(req), std::move(rep));
}

}


#endif /* HTTP_FILE_HANDLER_HH_ */
