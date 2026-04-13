/**************************************************************************/
/*  ai_requests.cpp                                                       */
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

#include "modules/woodot_ai/runtime/ai_requests.h"

#include "core/object/class_db.h"

void AICompletionRequest::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_model_rid", "model_rid"), &AICompletionRequest::set_model_rid);
	ClassDB::bind_method(D_METHOD("get_model_rid"), &AICompletionRequest::get_model_rid);
	ClassDB::bind_method(D_METHOD("set_prompt", "prompt"), &AICompletionRequest::set_prompt);
	ClassDB::bind_method(D_METHOD("get_prompt"), &AICompletionRequest::get_prompt);
	ClassDB::bind_method(D_METHOD("set_max_tokens", "max_tokens"), &AICompletionRequest::set_max_tokens);
	ClassDB::bind_method(D_METHOD("get_max_tokens"), &AICompletionRequest::get_max_tokens);
	ClassDB::bind_method(D_METHOD("set_temperature", "temperature"), &AICompletionRequest::set_temperature);
	ClassDB::bind_method(D_METHOD("get_temperature"), &AICompletionRequest::get_temperature);
	ClassDB::bind_method(D_METHOD("set_top_p", "top_p"), &AICompletionRequest::set_top_p);
	ClassDB::bind_method(D_METHOD("get_top_p"), &AICompletionRequest::get_top_p);
	ClassDB::bind_method(D_METHOD("set_top_k", "top_k"), &AICompletionRequest::set_top_k);
	ClassDB::bind_method(D_METHOD("get_top_k"), &AICompletionRequest::get_top_k);
	ClassDB::bind_method(D_METHOD("set_stream", "stream"), &AICompletionRequest::set_stream);
	ClassDB::bind_method(D_METHOD("is_streaming"), &AICompletionRequest::is_streaming);
	ClassDB::bind_method(D_METHOD("set_timeout_ms", "timeout_ms"), &AICompletionRequest::set_timeout_ms);
	ClassDB::bind_method(D_METHOD("get_timeout_ms"), &AICompletionRequest::get_timeout_ms);
	ClassDB::bind_method(D_METHOD("set_priority", "priority"), &AICompletionRequest::set_priority);
	ClassDB::bind_method(D_METHOD("get_priority"), &AICompletionRequest::get_priority);
	ClassDB::bind_method(D_METHOD("set_caller_tag", "caller_tag"), &AICompletionRequest::set_caller_tag);
	ClassDB::bind_method(D_METHOD("get_caller_tag"), &AICompletionRequest::get_caller_tag);
	ClassDB::bind_method(D_METHOD("set_metadata", "metadata"), &AICompletionRequest::set_metadata);
	ClassDB::bind_method(D_METHOD("get_metadata"), &AICompletionRequest::get_metadata);

	ADD_PROPERTY(PropertyInfo(Variant::RID, "model_rid"), "set_model_rid", "get_model_rid");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "prompt"), "set_prompt", "get_prompt");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_tokens"), "set_max_tokens", "get_max_tokens");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "temperature"), "set_temperature", "get_temperature");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "top_p"), "set_top_p", "get_top_p");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "top_k"), "set_top_k", "get_top_k");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "stream"), "set_stream", "is_streaming");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "timeout_ms"), "set_timeout_ms", "get_timeout_ms");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "priority"), "set_priority", "get_priority");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "caller_tag"), "set_caller_tag", "get_caller_tag");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "metadata"), "set_metadata", "get_metadata");
}

void AIEmbeddingRequest::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_model_rid", "model_rid"), &AIEmbeddingRequest::set_model_rid);
	ClassDB::bind_method(D_METHOD("get_model_rid"), &AIEmbeddingRequest::get_model_rid);
	ClassDB::bind_method(D_METHOD("set_inputs", "inputs"), &AIEmbeddingRequest::set_inputs);
	ClassDB::bind_method(D_METHOD("get_inputs"), &AIEmbeddingRequest::get_inputs);
	ClassDB::bind_method(D_METHOD("set_normalize", "normalize"), &AIEmbeddingRequest::set_normalize);
	ClassDB::bind_method(D_METHOD("is_normalized"), &AIEmbeddingRequest::is_normalized);
	ClassDB::bind_method(D_METHOD("set_timeout_ms", "timeout_ms"), &AIEmbeddingRequest::set_timeout_ms);
	ClassDB::bind_method(D_METHOD("get_timeout_ms"), &AIEmbeddingRequest::get_timeout_ms);
	ClassDB::bind_method(D_METHOD("set_priority", "priority"), &AIEmbeddingRequest::set_priority);
	ClassDB::bind_method(D_METHOD("get_priority"), &AIEmbeddingRequest::get_priority);
	ClassDB::bind_method(D_METHOD("set_caller_tag", "caller_tag"), &AIEmbeddingRequest::set_caller_tag);
	ClassDB::bind_method(D_METHOD("get_caller_tag"), &AIEmbeddingRequest::get_caller_tag);
	ClassDB::bind_method(D_METHOD("set_metadata", "metadata"), &AIEmbeddingRequest::set_metadata);
	ClassDB::bind_method(D_METHOD("get_metadata"), &AIEmbeddingRequest::get_metadata);

	ADD_PROPERTY(PropertyInfo(Variant::RID, "model_rid"), "set_model_rid", "get_model_rid");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "inputs"), "set_inputs", "get_inputs");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "normalize"), "set_normalize", "is_normalized");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "timeout_ms"), "set_timeout_ms", "get_timeout_ms");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "priority"), "set_priority", "get_priority");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "caller_tag"), "set_caller_tag", "get_caller_tag");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "metadata"), "set_metadata", "get_metadata");
}

#define AI_REQUEST_ACCESSORS(m_class, m_type, m_name) \
	void m_class::set_##m_name(const m_type &p_##m_name) { m_name = p_##m_name; } \
	m_type m_class::get_##m_name() const { return m_name; }

AI_REQUEST_ACCESSORS(AICompletionRequest, RID, model_rid)
AI_REQUEST_ACCESSORS(AICompletionRequest, String, prompt)
AI_REQUEST_ACCESSORS(AICompletionRequest, int32_t, max_tokens)
AI_REQUEST_ACCESSORS(AICompletionRequest, float, temperature)
AI_REQUEST_ACCESSORS(AICompletionRequest, float, top_p)
AI_REQUEST_ACCESSORS(AICompletionRequest, int32_t, top_k)
AI_REQUEST_ACCESSORS(AICompletionRequest, int32_t, timeout_ms)
AI_REQUEST_ACCESSORS(AICompletionRequest, int32_t, priority)
AI_REQUEST_ACCESSORS(AICompletionRequest, String, caller_tag)
AI_REQUEST_ACCESSORS(AICompletionRequest, Dictionary, metadata)

void AICompletionRequest::set_stream(bool p_stream) {
	stream = p_stream;
}

bool AICompletionRequest::is_streaming() const {
	return stream;
}

AI_REQUEST_ACCESSORS(AIEmbeddingRequest, RID, model_rid)
AI_REQUEST_ACCESSORS(AIEmbeddingRequest, PackedStringArray, inputs)
AI_REQUEST_ACCESSORS(AIEmbeddingRequest, int32_t, timeout_ms)
AI_REQUEST_ACCESSORS(AIEmbeddingRequest, int32_t, priority)
AI_REQUEST_ACCESSORS(AIEmbeddingRequest, String, caller_tag)
AI_REQUEST_ACCESSORS(AIEmbeddingRequest, Dictionary, metadata)

void AIEmbeddingRequest::set_normalize(bool p_normalize) {
	normalize = p_normalize;
}

bool AIEmbeddingRequest::is_normalized() const {
	return normalize;
}
