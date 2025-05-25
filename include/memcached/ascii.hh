
#line 1 "ascii.rl"
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

#include "../app/app-template.hh"
#include "memcached.hh"
#include <memory>
#include <algorithm>
#include <functional>


#line 93 "ascii.rl"


class memcache_ascii_parser : public ragel_parser_base<memcache_ascii_parser> {
    
#line 37 "ascii.hh"
inline static const char _actions[] = {
	0, 1, 0, 1, 2, 1, 4, 1, 
	6, 1, 7, 1, 8, 1, 9, 1, 
	10, 1, 11, 1, 12, 1, 15, 1, 
	16, 1, 17, 1, 18, 1, 19, 1, 
	20, 1, 22, 1, 24, 1, 25, 1, 
	26, 1, 27, 1, 28, 1, 29, 1, 
	30, 1, 31, 1, 32, 2, 1, 2, 
	2, 3, 4, 2, 5, 6, 2, 7, 
	12, 2, 7, 21, 2, 7, 23, 2, 
	9, 12, 2, 10, 12, 2, 13, 12, 
	2, 14, 12, 3, 0, 3, 4
};

inline static const short _key_offsets[] = {
	0, 0, 9, 10, 11, 12, 13, 14, 
	16, 19, 21, 24, 26, 30, 31, 32, 
	33, 34, 35, 36, 37, 38, 39, 40, 
	41, 42, 43, 44, 45, 46, 48, 51, 
	53, 56, 58, 61, 63, 67, 68, 69, 
	70, 71, 72, 73, 74, 75, 76, 77, 
	78, 79, 81, 82, 83, 84, 85, 87, 
	91, 92, 93, 94, 95, 96, 97, 98, 
	99, 100, 101, 102, 103, 104, 105, 107, 
	110, 111, 112, 113, 114, 115, 116, 117, 
	118, 119, 120, 121, 122, 123, 124, 125, 
	126, 127, 129, 130, 133, 137, 138, 139, 
	140, 141, 142, 143, 144, 145, 146, 147, 
	149, 150, 152, 155, 156, 157, 159, 162, 
	163, 164, 165, 166, 167, 168, 170, 174, 
	175, 176, 177, 178, 179, 180, 181, 182, 
	183, 184, 185, 186, 187, 188, 189, 190, 
	191, 192, 194, 197, 199, 202, 204, 208, 
	209, 210, 211, 212, 213, 214, 215, 216, 
	217, 218, 219, 221, 222, 223, 224, 225, 
	227, 230, 232, 235, 237, 241, 242, 243, 
	244, 245, 246, 247, 248, 249, 250, 251, 
	252, 253, 254, 255, 257, 258, 259, 260, 
	261, 262, 263, 264, 265, 266, 267, 268, 
	269, 270, 271, 272, 272, 272, 274, 276, 
	278
};

inline static const char _trans_keys[] = {
	97, 99, 100, 102, 103, 105, 114, 115, 
	118, 100, 100, 32, 32, 32, 48, 57, 
	32, 48, 57, 48, 57, 32, 48, 57, 
	48, 57, 13, 32, 48, 57, 10, 13, 
	10, 110, 111, 114, 101, 112, 108, 121, 
	13, 97, 115, 32, 32, 32, 48, 57, 
	32, 48, 57, 48, 57, 32, 48, 57, 
	48, 57, 32, 48, 57, 48, 57, 13, 
	32, 48, 57, 10, 13, 10, 110, 111, 
	114, 101, 112, 108, 121, 13, 101, 99, 
	108, 114, 32, 32, 32, 48, 57, 13, 
	32, 48, 57, 10, 110, 111, 114, 101, 
	112, 108, 121, 13, 101, 116, 101, 32, 
	32, 13, 32, 10, 13, 32, 110, 111, 
	114, 101, 112, 108, 121, 13, 10, 108, 
	117, 115, 104, 95, 97, 108, 108, 13, 
	32, 10, 110, 48, 57, 13, 32, 48, 
	57, 110, 111, 114, 101, 112, 108, 121, 
	13, 101, 116, 32, 115, 32, 13, 32, 
	10, 13, 32, 32, 32, 13, 32, 10, 
	13, 32, 110, 99, 114, 32, 32, 32, 
	48, 57, 13, 32, 48, 57, 10, 110, 
	111, 114, 101, 112, 108, 121, 13, 101, 
	112, 108, 97, 99, 101, 32, 32, 32, 
	48, 57, 32, 48, 57, 48, 57, 32, 
	48, 57, 48, 57, 13, 32, 48, 57, 
	10, 13, 10, 110, 111, 114, 101, 112, 
	108, 121, 13, 101, 116, 116, 32, 32, 
	32, 48, 57, 32, 48, 57, 48, 57, 
	32, 48, 57, 48, 57, 13, 32, 48, 
	57, 10, 13, 10, 110, 111, 114, 101, 
	112, 108, 121, 13, 97, 116, 115, 13, 
	32, 10, 104, 97, 115, 104, 13, 10, 
	101, 114, 115, 105, 111, 110, 13, 10, 
	13, 32, 13, 32, 13, 32, 0
};
inline static const char _single_lengths[] = {
	0, 9, 1, 1, 1, 1, 1, 0, 
	1, 0, 1, 0, 2, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 0, 1, 0, 
	1, 0, 1, 0, 2, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 2, 1, 1, 1, 1, 0, 2, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 2, 3, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 2, 1, 1, 2, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 2, 
	1, 2, 3, 1, 1, 2, 3, 1, 
	1, 1, 1, 1, 1, 0, 2, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 0, 1, 0, 1, 0, 2, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 2, 1, 1, 1, 1, 0, 
	1, 0, 1, 0, 2, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 2, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 0, 0, 2, 2, 2, 
	0
};
inline static const char _range_lengths[] = {
	0, 0, 0, 0, 0, 0, 0, 1, 
	1, 1, 1, 1, 1, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 1, 1, 1, 
	1, 1, 1, 1, 1, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 1, 1, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 1, 1, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 1, 1, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 1, 1, 1, 1, 1, 1, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 1, 
	1, 1, 1, 1, 1, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0
};
inline static const short _index_offsets[] = {
	0, 0, 10, 12, 14, 16, 18, 20, 
	22, 25, 27, 30, 32, 36, 38, 40, 
	42, 44, 46, 48, 50, 52, 54, 56, 
	58, 60, 62, 64, 66, 68, 70, 73, 
	75, 78, 80, 83, 85, 89, 91, 93, 
	95, 97, 99, 101, 103, 105, 107, 109, 
	111, 113, 116, 118, 120, 122, 124, 126, 
	130, 132, 134, 136, 138, 140, 142, 144, 
	146, 148, 150, 152, 154, 156, 158, 161, 
	165, 167, 169, 171, 173, 175, 177, 179, 
	181, 183, 185, 187, 189, 191, 193, 195, 
	197, 199, 202, 204, 207, 211, 213, 215, 
	217, 219, 221, 223, 225, 227, 229, 231, 
	234, 236, 239, 243, 245, 247, 250, 254, 
	256, 258, 260, 262, 264, 266, 268, 272, 
	274, 276, 278, 280, 282, 284, 286, 288, 
	290, 292, 294, 296, 298, 300, 302, 304, 
	306, 308, 310, 313, 315, 318, 320, 324, 
	326, 328, 330, 332, 334, 336, 338, 340, 
	342, 344, 346, 349, 351, 353, 355, 357, 
	359, 362, 364, 367, 369, 373, 375, 377, 
	379, 381, 383, 385, 387, 389, 391, 393, 
	395, 397, 399, 401, 404, 406, 408, 410, 
	412, 414, 416, 418, 420, 422, 424, 426, 
	428, 430, 432, 434, 435, 436, 439, 442, 
	445
};
inline static const unsigned char _trans_targs[] = {
	2, 24, 48, 81, 101, 111, 128, 154, 
	187, 0, 3, 0, 4, 0, 5, 0, 
	0, 6, 7, 6, 8, 0, 9, 8, 
	0, 10, 0, 11, 10, 0, 12, 0, 
	13, 16, 12, 0, 14, 0, 15, 0, 
	196, 0, 17, 0, 18, 0, 19, 0, 
	20, 0, 21, 0, 22, 0, 23, 0, 
	13, 0, 25, 0, 26, 0, 27, 0, 
	0, 28, 29, 28, 30, 0, 31, 30, 
	0, 32, 0, 33, 32, 0, 34, 0, 
	35, 34, 0, 36, 0, 37, 40, 36, 
	0, 38, 0, 39, 0, 196, 0, 41, 
	0, 42, 0, 43, 0, 44, 0, 45, 
	0, 46, 0, 47, 0, 37, 0, 49, 
	0, 50, 65, 0, 51, 0, 52, 0, 
	0, 53, 54, 53, 55, 0, 56, 57, 
	55, 0, 196, 0, 58, 0, 59, 0, 
	60, 0, 61, 0, 62, 0, 63, 0, 
	64, 0, 56, 0, 66, 0, 67, 0, 
	68, 0, 69, 0, 0, 70, 71, 72, 
	70, 197, 71, 72, 70, 73, 0, 74, 
	0, 75, 0, 76, 0, 77, 0, 78, 
	0, 79, 0, 80, 0, 196, 0, 82, 
	0, 83, 0, 84, 0, 85, 0, 86, 
	0, 87, 0, 88, 0, 89, 0, 90, 
	91, 0, 196, 0, 94, 92, 0, 90, 
	93, 92, 0, 94, 0, 95, 0, 96, 
	0, 97, 0, 98, 0, 99, 0, 100, 
	0, 90, 0, 102, 0, 103, 0, 104, 
	107, 0, 0, 105, 106, 104, 105, 198, 
	106, 104, 105, 108, 0, 0, 109, 110, 
	108, 109, 199, 110, 108, 109, 112, 0, 
	113, 0, 114, 0, 115, 0, 0, 116, 
	117, 116, 118, 0, 119, 120, 118, 0, 
	196, 0, 121, 0, 122, 0, 123, 0, 
	124, 0, 125, 0, 126, 0, 127, 0, 
	119, 0, 129, 0, 130, 0, 131, 0, 
	132, 0, 133, 0, 134, 0, 135, 0, 
	0, 136, 137, 136, 138, 0, 139, 138, 
	0, 140, 0, 141, 140, 0, 142, 0, 
	143, 146, 142, 0, 144, 0, 145, 0, 
	196, 0, 147, 0, 148, 0, 149, 0, 
	150, 0, 151, 0, 152, 0, 153, 0, 
	143, 0, 155, 176, 0, 156, 0, 157, 
	0, 0, 158, 159, 158, 160, 0, 161, 
	160, 0, 162, 0, 163, 162, 0, 164, 
	0, 165, 168, 164, 0, 166, 0, 167, 
	0, 196, 0, 169, 0, 170, 0, 171, 
	0, 172, 0, 173, 0, 174, 0, 175, 
	0, 165, 0, 177, 0, 178, 0, 179, 
	0, 180, 181, 0, 196, 0, 182, 0, 
	183, 0, 184, 0, 185, 0, 186, 0, 
	196, 0, 188, 0, 189, 0, 190, 0, 
	191, 0, 192, 0, 193, 0, 194, 0, 
	196, 0, 200, 0, 71, 72, 70, 106, 
	104, 105, 110, 108, 109, 200, 0
};
inline static const char _trans_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 1, 9, 0, 1, 0, 11, 0, 
	0, 56, 0, 13, 5, 0, 83, 0, 
	74, 74, 5, 0, 21, 0, 0, 0, 
	25, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 17, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 1, 9, 0, 1, 0, 11, 0, 
	0, 56, 0, 13, 5, 0, 83, 0, 
	15, 5, 0, 59, 0, 80, 80, 7, 
	0, 29, 0, 0, 0, 31, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 17, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 1, 9, 0, 59, 0, 19, 19, 
	7, 0, 49, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	17, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 1, 62, 62, 
	0, 37, 62, 62, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 17, 0, 0, 0, 37, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 77, 
	77, 0, 39, 0, 0, 56, 0, 71, 
	71, 5, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 17, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 1, 65, 65, 0, 33, 
	65, 65, 0, 0, 0, 0, 1, 68, 
	68, 0, 35, 68, 68, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 1, 
	9, 0, 59, 0, 19, 19, 7, 0, 
	47, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 17, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 1, 9, 0, 1, 0, 11, 0, 
	0, 56, 0, 13, 5, 0, 83, 0, 
	74, 74, 5, 0, 21, 0, 0, 0, 
	27, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 17, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 1, 9, 0, 1, 0, 11, 
	0, 0, 56, 0, 13, 5, 0, 83, 
	0, 74, 74, 5, 0, 21, 0, 0, 
	0, 23, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 17, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 43, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	45, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	41, 0, 53, 0, 62, 62, 0, 65, 
	65, 0, 68, 68, 0, 3, 0
};
inline 
static const char _eof_actions[] = {
	0, 51, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0
};

inline static const int start = 1;
inline static const int error = 0;

inline static const int en_blob = 195;
inline static const int en_main = 1;


#line 97 "ascii.rl"
public:
    enum class state {
        error,
        eof,
        cmd_set,
        cmd_cas,
        cmd_add,
        cmd_replace,
        cmd_get,
        cmd_gets,
        cmd_delete,
        cmd_flush_all,
        cmd_version,
        cmd_stats,
        cmd_stats_hash,
        cmd_incr,
        cmd_decr,
    };
    state _state;
    uint32_t _u32;
    uint64_t _u64;
    memcache::item_key _key;
    sstring _flags_str;
    uint32_t _expiration;
    uint32_t _size;
    sstring _size_str;
    uint32_t _size_left;
    uint64_t _version;
    sstring _blob;
    bool _noreply;
    std::vector<memcache::item_key> _keys;
public:
    void init() {
        init_base();
        _state = state::error;
        _keys.clear();
        
#line 398 "ascii.hh"
	{
	 _fsm_cs = start;
	 _fsm_top = 0;
	}

#line 134 "ascii.rl"
    }

    char* parse(char* p, char* pe, char* eof) {
        string_builder::guard g(_builder, p, pe);
        auto str = [this, &g, &p] { g.mark_end(p); return get_str(); };
        
#line 411 "ascii.hh"
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
#line 34 "ascii.rl"
	{
    g.mark_start(p);
}
	break;
	case 1:
#line 38 "ascii.rl"
	{
    g.mark_start(p);
    _size_left = _size;
}
	break;
	case 2:
#line 43 "ascii.rl"
	{
    auto len = std::min((uint32_t)(pe - p), _size_left);
    _size_left -= len;
    p += len;
    if (_size_left == 0) {
        _blob = str();
        p--;
        { _fsm_cs =  _fsm_stack[-- _fsm_top]; {
    postpop();
}goto _again;}
    }
    p--;
}
	break;
	case 3:
#line 57 "ascii.rl"
	{ _u32 = 0; }
	break;
	case 4:
#line 57 "ascii.rl"
	{ _u32 *= 10; _u32 += (*p) - '0'; }
	break;
	case 5:
#line 58 "ascii.rl"
	{ _u64 = 0; }
	break;
	case 6:
#line 58 "ascii.rl"
	{ _u64 *= 10; _u64 += (*p) - '0'; }
	break;
	case 7:
#line 59 "ascii.rl"
	{ _key = memcache::item_key(str()); }
	break;
	case 8:
#line 60 "ascii.rl"
	{ _flags_str = str(); }
	break;
	case 9:
#line 61 "ascii.rl"
	{ _expiration = _u32; }
	break;
	case 10:
#line 62 "ascii.rl"
	{ _size = _u32; _size_str = str(); }
	break;
	case 11:
#line 64 "ascii.rl"
	{ _noreply = true; }
	break;
	case 12:
#line 64 "ascii.rl"
	{ _noreply = false; }
	break;
	case 13:
#line 65 "ascii.rl"
	{ _expiration = 0; }
	break;
	case 14:
#line 66 "ascii.rl"
	{ _version = _u64; }
	break;
	case 15:
#line 68 "ascii.rl"
	{ {
    prepush();
{ _fsm_stack[ _fsm_top++] =  _fsm_cs;  _fsm_cs = 195;goto _again;}} }
	break;
	case 16:
#line 69 "ascii.rl"
	{ _state = state::cmd_set; }
	break;
	case 17:
#line 70 "ascii.rl"
	{ _state = state::cmd_add; }
	break;
	case 18:
#line 71 "ascii.rl"
	{ _state = state::cmd_replace; }
	break;
	case 19:
#line 72 "ascii.rl"
	{ {
    prepush();
{ _fsm_stack[ _fsm_top++] =  _fsm_cs;  _fsm_cs = 195;goto _again;}} }
	break;
	case 20:
#line 72 "ascii.rl"
	{ _state = state::cmd_cas; }
	break;
	case 21:
#line 73 "ascii.rl"
	{ this->_keys.emplace_back(std::move(_key)); }
	break;
	case 22:
#line 73 "ascii.rl"
	{ _state = state::cmd_get; }
	break;
	case 23:
#line 74 "ascii.rl"
	{ this->_keys.emplace_back(std::move(_key)); }
	break;
	case 24:
#line 74 "ascii.rl"
	{ _state = state::cmd_gets; }
	break;
	case 25:
#line 75 "ascii.rl"
	{ _state = state::cmd_delete; }
	break;
	case 26:
#line 76 "ascii.rl"
	{ _state = state::cmd_flush_all; }
	break;
	case 27:
#line 77 "ascii.rl"
	{ _state = state::cmd_version; }
	break;
	case 28:
#line 78 "ascii.rl"
	{ _state = state::cmd_stats; }
	break;
	case 29:
#line 79 "ascii.rl"
	{ _state = state::cmd_stats_hash; }
	break;
	case 30:
#line 80 "ascii.rl"
	{ _state = state::cmd_incr; }
	break;
	case 31:
#line 81 "ascii.rl"
	{ _state = state::cmd_decr; }
	break;
#line 633 "ascii.hh"
		}
	}

_again:
	if (  _fsm_cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	const char *__acts = _actions + _eof_actions[ _fsm_cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 32:
#line 83 "ascii.rl"
	{ _state = state::eof; }
	break;
#line 653 "ascii.hh"
		}
	}
	}

	_out: {}
	}

#line 140 "ascii.rl"
        if (_state != state::error) {
            return p;
        }
        if (p != pe) {
            p = pe;
            return p;
        }
        return nullptr;
    }
    bool eof() const {
        return _state == state::eof;
    }
};
