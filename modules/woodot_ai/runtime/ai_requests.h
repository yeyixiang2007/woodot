/**************************************************************************/
/*  ai_requests.h                                                         */
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

#include "core/object/ref_counted.h"
#include "core/templates/rid.h"
#include "core/variant/dictionary.h"
#include "core/variant/variant.h"

class AICompletionRequest : public RefCounted {
	GDCLASS(AICompletionRequest, RefCounted);

	RID model_rid;
	String prompt;
	int32_t max_tokens = 256;
	float temperature = 0.8f;
	float top_p = 0.95f;
	int32_t top_k = 40;
	bool stream = false;
	int32_t timeout_ms = 0;
	int32_t priority = 0;
	String caller_tag;
	Dictionary metadata;

protected:
	static void _bind_methods();

public:
	void set_model_rid(const RID &p_model_rid);
	RID get_model_rid() const;
	void set_prompt(const String &p_prompt);
	String get_prompt() const;
	void set_max_tokens(int32_t p_max_tokens);
	int32_t get_max_tokens() const;
	void set_temperature(float p_temperature);
	float get_temperature() const;
	void set_top_p(float p_top_p);
	float get_top_p() const;
	void set_top_k(int32_t p_top_k);
	int32_t get_top_k() const;
	void set_stream(bool p_stream);
	bool is_streaming() const;
	void set_timeout_ms(int32_t p_timeout_ms);
	int32_t get_timeout_ms() const;
	void set_priority(int32_t p_priority);
	int32_t get_priority() const;
	void set_caller_tag(const String &p_caller_tag);
	String get_caller_tag() const;
	void set_metadata(const Dictionary &p_metadata);
	Dictionary get_metadata() const;
};

class AIEmbeddingRequest : public RefCounted {
	GDCLASS(AIEmbeddingRequest, RefCounted);

	RID model_rid;
	PackedStringArray inputs;
	bool normalize = false;
	int32_t timeout_ms = 0;
	int32_t priority = 0;
	String caller_tag;
	Dictionary metadata;

protected:
	static void _bind_methods();

public:
	void set_model_rid(const RID &p_model_rid);
	RID get_model_rid() const;
	void set_inputs(const PackedStringArray &p_inputs);
	PackedStringArray get_inputs() const;
	void set_normalize(bool p_normalize);
	bool is_normalized() const;
	void set_timeout_ms(int32_t p_timeout_ms);
	int32_t get_timeout_ms() const;
	void set_priority(int32_t p_priority);
	int32_t get_priority() const;
	void set_caller_tag(const String &p_caller_tag);
	String get_caller_tag() const;
	void set_metadata(const Dictionary &p_metadata);
	Dictionary get_metadata() const;
};
