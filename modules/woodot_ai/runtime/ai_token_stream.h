/**************************************************************************/
/*  ai_token_stream.h                                                     */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "core/variant/dictionary.h"
#include "core/variant/variant.h"

class AITokenStream {
public:
	struct Config {
		bool enabled = false;
		int32_t flush_token_count = 8;
		int32_t flush_char_count = 96;
		uint64_t flush_interval_us = 16000;
	};

private:
	Config config;
	PackedStringArray pending_tokens;
	uint64_t total_tokens = 0;
	uint64_t total_characters = 0;
	uint64_t flush_count = 0;
	uint64_t last_push_tick_us = 0;
	uint64_t last_flush_tick_us = 0;

	int32_t _get_pending_character_count() const;

public:
	void configure(const Config &p_config);
	const Config &get_config() const;

	bool is_enabled() const;
	bool has_pending_tokens() const;
	int32_t get_pending_token_count() const;
	uint64_t get_total_tokens() const;
	uint64_t get_flush_count() const;

	void push_tokens(const PackedStringArray &p_tokens, uint64_t p_now_us);
	bool should_flush(uint64_t p_now_us) const;
	PackedStringArray flush(uint64_t p_now_us);
	PackedStringArray finalize(uint64_t p_now_us);
	void reset();

	Dictionary get_stats() const;
};
