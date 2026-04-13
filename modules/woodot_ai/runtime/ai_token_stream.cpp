/**************************************************************************/
/*  ai_token_stream.cpp                                                   */
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

#include "modules/woodot_ai/runtime/ai_token_stream.h"

int32_t AITokenStream::_get_pending_character_count() const {
	int32_t count = 0;
	for (int i = 0; i < pending_tokens.size(); i++) {
		count += pending_tokens[i].length();
	}
	return count;
}

void AITokenStream::configure(const Config &p_config) {
	config = p_config;
}

const AITokenStream::Config &AITokenStream::get_config() const {
	return config;
}

bool AITokenStream::is_enabled() const {
	return config.enabled;
}

bool AITokenStream::has_pending_tokens() const {
	return !pending_tokens.is_empty();
}

int32_t AITokenStream::get_pending_token_count() const {
	return pending_tokens.size();
}

uint64_t AITokenStream::get_total_tokens() const {
	return total_tokens;
}

uint64_t AITokenStream::get_flush_count() const {
	return flush_count;
}

void AITokenStream::push_tokens(const PackedStringArray &p_tokens, uint64_t p_now_us) {
	if (p_tokens.is_empty()) {
		return;
	}

	last_push_tick_us = p_now_us;
	for (int i = 0; i < p_tokens.size(); i++) {
		pending_tokens.push_back(p_tokens[i]);
		total_tokens++;
		total_characters += static_cast<uint64_t>(p_tokens[i].length());
	}
}

bool AITokenStream::should_flush(uint64_t p_now_us) const {
	if (!config.enabled || pending_tokens.is_empty()) {
		return false;
	}

	if (config.flush_token_count > 0 && pending_tokens.size() >= config.flush_token_count) {
		return true;
	}

	if (config.flush_char_count > 0 && _get_pending_character_count() >= config.flush_char_count) {
		return true;
	}

	if (config.flush_interval_us > 0 && last_push_tick_us > 0 && p_now_us >= last_push_tick_us && (p_now_us - last_push_tick_us) >= config.flush_interval_us) {
		return true;
	}

	return false;
}

PackedStringArray AITokenStream::flush(uint64_t p_now_us) {
	PackedStringArray flushed = pending_tokens;
	if (!flushed.is_empty()) {
		flush_count++;
		last_flush_tick_us = p_now_us;
	}
	pending_tokens.clear();
	return flushed;
}

PackedStringArray AITokenStream::finalize(uint64_t p_now_us) {
	return flush(p_now_us);
}

void AITokenStream::reset() {
	pending_tokens.clear();
	total_tokens = 0;
	total_characters = 0;
	flush_count = 0;
	last_push_tick_us = 0;
	last_flush_tick_us = 0;
}

Dictionary AITokenStream::get_stats() const {
	Dictionary stats;
	stats["enabled"] = config.enabled;
	stats["pending_tokens"] = static_cast<int64_t>(pending_tokens.size());
	stats["pending_characters"] = static_cast<int64_t>(_get_pending_character_count());
	stats["total_tokens"] = static_cast<int64_t>(total_tokens);
	stats["total_characters"] = static_cast<int64_t>(total_characters);
	stats["flush_count"] = static_cast<int64_t>(flush_count);
	stats["last_push_tick_us"] = static_cast<int64_t>(last_push_tick_us);
	stats["last_flush_tick_us"] = static_cast<int64_t>(last_flush_tick_us);
	stats["flush_token_count"] = static_cast<int64_t>(config.flush_token_count);
	stats["flush_char_count"] = static_cast<int64_t>(config.flush_char_count);
	stats["flush_interval_us"] = static_cast<int64_t>(config.flush_interval_us);
	return stats;
}
