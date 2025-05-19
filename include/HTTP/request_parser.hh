
#line 1 "request_parser.rl"
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

#pragma once

#include <memory>
#include <unordered_map>
#include "request.hh"
#include "../../include/future/future_all12.hh"

using namespace httpd;


#line 32 "request_parser.rl"


#line 97 "request_parser.rl"


class http_request_parser : public ragel_parser_base<http_request_parser> {
    
#line 43 "request_parser.hh"
inline static const char _actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 5, 1, 6, 1, 
	7, 1, 8, 2, 0, 5, 2, 4, 
	5, 2, 5, 7, 2, 6, 0, 2, 
	6, 7, 2, 7, 0, 3, 6, 5, 
	7, 3, 6, 7, 0
};

inline static const unsigned char _key_offsets[] = {
	0, 0, 2, 5, 7, 9, 12, 13, 
	14, 15, 16, 17, 19, 20, 22, 23, 
	24, 40, 41, 59, 62, 65, 66, 67, 
	85, 88, 91, 95, 113, 132, 136, 139, 
	142, 146, 164
};

inline static const char _trans_keys[] = {
	65, 90, 32, 65, 90, 13, 32, 13, 
	32, 10, 13, 32, 72, 84, 84, 80, 
	47, 48, 57, 46, 48, 57, 13, 10, 
	13, 33, 124, 126, 35, 39, 42, 43, 
	45, 46, 48, 57, 65, 90, 94, 122, 
	10, 9, 32, 33, 58, 124, 126, 35, 
	39, 42, 43, 45, 46, 48, 57, 65, 
	90, 94, 122, 9, 32, 58, 9, 13, 
	32, 13, 10, 9, 13, 32, 33, 124, 
	126, 35, 39, 42, 43, 45, 46, 48, 
	57, 65, 90, 94, 122, 9, 13, 32, 
	9, 13, 32, 9, 10, 13, 32, 9, 
	13, 32, 33, 124, 126, 35, 39, 42, 
	43, 45, 46, 48, 57, 65, 90, 94, 
	122, 9, 13, 32, 33, 58, 124, 126, 
	35, 39, 42, 43, 45, 46, 48, 57, 
	65, 90, 94, 122, 9, 13, 32, 58, 
	9, 13, 32, 9, 13, 32, 9, 10, 
	13, 32, 9, 13, 32, 33, 124, 126, 
	35, 39, 42, 43, 45, 46, 48, 57, 
	65, 90, 94, 122, 0
};

inline static const char _single_lengths[] = {
	0, 0, 1, 2, 2, 3, 1, 1, 
	1, 1, 1, 0, 1, 0, 1, 1, 
	4, 1, 6, 3, 3, 1, 1, 6, 
	3, 3, 4, 6, 7, 4, 3, 3, 
	4, 6, 0
};

inline static const char _range_lengths[] = {
	0, 1, 1, 0, 0, 0, 0, 0, 
	0, 0, 0, 1, 0, 1, 0, 0, 
	6, 0, 6, 0, 0, 0, 0, 6, 
	0, 0, 0, 6, 6, 0, 0, 0, 
	0, 6, 0
};

inline static const unsigned char _index_offsets[] = {
	0, 0, 2, 5, 8, 11, 15, 17, 
	19, 21, 23, 25, 27, 29, 31, 33, 
	35, 46, 48, 61, 65, 69, 71, 73, 
	86, 90, 94, 99, 112, 126, 131, 135, 
	139, 144, 157
};

inline static const char _indicies[] = {
	0, 1, 2, 3, 1, 5, 1, 4, 
	7, 8, 6, 1, 7, 8, 6, 9, 
	1, 10, 1, 11, 1, 12, 1, 13, 
	1, 14, 1, 15, 1, 16, 1, 17, 
	1, 18, 1, 19, 20, 20, 20, 20, 
	20, 20, 20, 20, 20, 1, 21, 1, 
	22, 22, 23, 24, 23, 23, 23, 23, 
	23, 23, 23, 23, 1, 25, 25, 26, 
	1, 28, 29, 28, 27, 31, 30, 32, 
	1, 33, 34, 33, 35, 35, 35, 35, 
	35, 35, 35, 35, 35, 1, 37, 38, 
	37, 36, 40, 41, 40, 39, 40, 42, 
	41, 40, 39, 43, 44, 43, 45, 45, 
	45, 45, 45, 45, 45, 45, 45, 39, 
	46, 41, 46, 47, 48, 47, 47, 47, 
	47, 47, 47, 47, 47, 39, 49, 41, 
	49, 50, 39, 52, 53, 52, 51, 55, 
	56, 55, 54, 40, 57, 41, 40, 39, 
	58, 59, 58, 60, 60, 60, 60, 60, 
	60, 60, 60, 60, 39, 1, 0
};

inline static const char _trans_targs[] = {
	2, 0, 3, 2, 4, 5, 4, 5, 
	6, 7, 8, 9, 10, 11, 12, 13, 
	14, 15, 16, 17, 18, 34, 19, 18, 
	20, 19, 20, 21, 20, 22, 21, 22, 
	23, 24, 17, 18, 25, 24, 26, 25, 
	25, 26, 27, 24, 17, 28, 29, 28, 
	30, 29, 30, 31, 30, 32, 31, 31, 
	32, 33, 24, 17, 28
};

inline static const char _trans_actions[] = {
	1, 0, 3, 0, 1, 1, 0, 0, 
	5, 0, 0, 0, 0, 0, 1, 0, 
	0, 7, 0, 0, 1, 17, 9, 0, 
	9, 0, 0, 1, 1, 19, 0, 11, 
	0, 13, 13, 28, 1, 19, 19, 0, 
	11, 11, 0, 25, 15, 34, 22, 0, 
	9, 11, 0, 1, 19, 19, 0, 11, 
	11, 0, 37, 31, 41
};

inline static const int start = 1;
inline static const int error = 0;

inline static const int en_main = 1;


#line 101 "request_parser.rl"
public:
    enum class state {
        error,
        eof,
        done,
    };
    std::unique_ptr<httpd::request> _req;
    sstring _field_name;
    sstring _value;
    state _state;
public:
    void init() {
        init_base();
        _req.reset(new httpd::request());
        _state = state::eof;
        
#line 177 "request_parser.hh"
	{
	 _fsm_cs = start;
	}

#line 117 "request_parser.rl"
    }
    char* parse(char* p, char* pe, char* eof) {
        sstring_builder::guard g(_builder, p, pe);
        auto str = [this, &g, &p] { g.mark_end(p); return get_str(); };
        bool done = false;
        if (p != pe) {
            _state = state::error;
        }
        
#line 192 "request_parser.hh"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _test_eof;
	if (  _fsm_cs == 0 )
		goto _out;
_resume:
	_keys = _trans_keys + _key_offsets[ _fsm_cs];
	_trans = _index_offsets[ _fsm_cs];

	_klen = _single_lengths[ _fsm_cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _range_lengths[ _fsm_cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	_trans = _indicies[_trans];
	 _fsm_cs = _trans_targs[_trans];

	if ( _trans_actions[_trans] == 0 )
		goto _again;

	_acts = _actions + _trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 37 "request_parser.rl"
	{
    g.mark_start(p);
}
	break;
	case 1:
#line 41 "request_parser.rl"
	{
    _req->_method = str();
}
	break;
	case 2:
#line 45 "request_parser.rl"
	{
    _req->_url = str();
}
	break;
	case 3:
#line 49 "request_parser.rl"
	{
    _req->_version = str();
}
	break;
	case 4:
#line 53 "request_parser.rl"
	{
    _field_name = str();
}
	break;
	case 5:
#line 57 "request_parser.rl"
	{
    _value = str();
}
	break;
	case 6:
#line 61 "request_parser.rl"
	{
    _req->_headers[_field_name] = std::move(_value);
}
	break;
	case 7:
#line 65 "request_parser.rl"
	{
    _req->_headers[_field_name] += sstring(" ") + std::move(_value);
}
	break;
	case 8:
#line 69 "request_parser.rl"
	{
    done = true;
    {p++; goto _out; }
}
	break;
#line 321 "request_parser.hh"
		}
	}

_again:
	if (  _fsm_cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	_out: {}
	}

#line 126 "request_parser.rl"
        if (!done) {
            p = nullptr;
        } else {
            _state = state::done;
        }
        return p;
    }
    auto get_parsed_request() {
        return std::move(_req);
    }
    bool eof() const {
        return _state == state::eof;
    }
};
