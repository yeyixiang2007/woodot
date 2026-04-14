/**************************************************************************/
/*  ai_request_resources.cpp                                              */
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

#include "modules/woodot_ai/resources/ai_request_resources.h"

#include "core/object/class_db.h"

void AICompletionRequestResource::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_prompt", "prompt"), &AICompletionRequestResource::set_prompt);
	ClassDB::bind_method(D_METHOD("get_prompt"), &AICompletionRequestResource::get_prompt);
	ClassDB::bind_method(D_METHOD("set_max_tokens", "max_tokens"), &AICompletionRequestResource::set_max_tokens);
	ClassDB::bind_method(D_METHOD("get_max_tokens"), &AICompletionRequestResource::get_max_tokens);
	ClassDB::bind_method(D_METHOD("set_temperature", "temperature"), &AICompletionRequestResource::set_temperature);
	ClassDB::bind_method(D_METHOD("get_temperature"), &AICompletionRequestResource::get_temperature);
	ClassDB::bind_method(D_METHOD("set_top_p", "top_p"), &AICompletionRequestResource::set_top_p);
	ClassDB::bind_method(D_METHOD("get_top_p"), &AICompletionRequestResource::get_top_p);
	ClassDB::bind_method(D_METHOD("set_top_k", "top_k"), &AICompletionRequestResource::set_top_k);
	ClassDB::bind_method(D_METHOD("get_top_k"), &AICompletionRequestResource::get_top_k);
	ClassDB::bind_method(D_METHOD("set_stream", "stream"), &AICompletionRequestResource::set_stream);
	ClassDB::bind_method(D_METHOD("is_streaming"), &AICompletionRequestResource::is_streaming);
	ClassDB::bind_method(D_METHOD("set_timeout_ms", "timeout_ms"), &AICompletionRequestResource::set_timeout_ms);
	ClassDB::bind_method(D_METHOD("get_timeout_ms"), &AICompletionRequestResource::get_timeout_ms);
	ClassDB::bind_method(D_METHOD("set_priority", "priority"), &AICompletionRequestResource::set_priority);
	ClassDB::bind_method(D_METHOD("get_priority"), &AICompletionRequestResource::get_priority);
	ClassDB::bind_method(D_METHOD("set_caller_tag", "caller_tag"), &AICompletionRequestResource::set_caller_tag);
	ClassDB::bind_method(D_METHOD("get_caller_tag"), &AICompletionRequestResource::get_caller_tag);
	ClassDB::bind_method(D_METHOD("set_metadata", "metadata"), &AICompletionRequestResource::set_metadata);
	ClassDB::bind_method(D_METHOD("get_metadata"), &AICompletionRequestResource::get_metadata);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "prompt", PROPERTY_HINT_MULTILINE_TEXT), "set_prompt", "get_prompt");
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

void AIEmbeddingRequestResource::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_inputs", "inputs"), &AIEmbeddingRequestResource::set_inputs);
	ClassDB::bind_method(D_METHOD("get_inputs"), &AIEmbeddingRequestResource::get_inputs);
	ClassDB::bind_method(D_METHOD("set_normalize", "normalize"), &AIEmbeddingRequestResource::set_normalize);
	ClassDB::bind_method(D_METHOD("is_normalized"), &AIEmbeddingRequestResource::is_normalized);
	ClassDB::bind_method(D_METHOD("set_timeout_ms", "timeout_ms"), &AIEmbeddingRequestResource::set_timeout_ms);
	ClassDB::bind_method(D_METHOD("get_timeout_ms"), &AIEmbeddingRequestResource::get_timeout_ms);
	ClassDB::bind_method(D_METHOD("set_priority", "priority"), &AIEmbeddingRequestResource::set_priority);
	ClassDB::bind_method(D_METHOD("get_priority"), &AIEmbeddingRequestResource::get_priority);
	ClassDB::bind_method(D_METHOD("set_caller_tag", "caller_tag"), &AIEmbeddingRequestResource::set_caller_tag);
	ClassDB::bind_method(D_METHOD("get_caller_tag"), &AIEmbeddingRequestResource::get_caller_tag);
	ClassDB::bind_method(D_METHOD("set_metadata", "metadata"), &AIEmbeddingRequestResource::set_metadata);
	ClassDB::bind_method(D_METHOD("get_metadata"), &AIEmbeddingRequestResource::get_metadata);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "inputs"), "set_inputs", "get_inputs");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "normalize"), "set_normalize", "is_normalized");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "timeout_ms"), "set_timeout_ms", "get_timeout_ms");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "priority"), "set_priority", "get_priority");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "caller_tag"), "set_caller_tag", "get_caller_tag");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "metadata"), "set_metadata", "get_metadata");
}

#define AI_REQUEST_RESOURCE_REF_ACCESSORS(m_class, m_type, m_name) \
	void m_class::set_##m_name(const m_type &p_##m_name) { m_name = p_##m_name; } \
	m_type m_class::get_##m_name() const { return m_name; }

AI_REQUEST_RESOURCE_REF_ACCESSORS(AICompletionRequestResource, String, prompt)
AI_REQUEST_RESOURCE_REF_ACCESSORS(AICompletionRequestResource, String, caller_tag)
AI_REQUEST_RESOURCE_REF_ACCESSORS(AICompletionRequestResource, Dictionary, metadata)

void AICompletionRequestResource::set_max_tokens(int32_t p_max_tokens) {
	max_tokens = p_max_tokens;
}

int32_t AICompletionRequestResource::get_max_tokens() const {
	return max_tokens;
}

void AICompletionRequestResource::set_temperature(float p_temperature) {
	temperature = p_temperature;
}

float AICompletionRequestResource::get_temperature() const {
	return temperature;
}

void AICompletionRequestResource::set_top_p(float p_top_p) {
	top_p = p_top_p;
}

float AICompletionRequestResource::get_top_p() const {
	return top_p;
}

void AICompletionRequestResource::set_top_k(int32_t p_top_k) {
	top_k = p_top_k;
}

int32_t AICompletionRequestResource::get_top_k() const {
	return top_k;
}

void AICompletionRequestResource::set_timeout_ms(int32_t p_timeout_ms) {
	timeout_ms = p_timeout_ms;
}

int32_t AICompletionRequestResource::get_timeout_ms() const {
	return timeout_ms;
}

void AICompletionRequestResource::set_priority(int32_t p_priority) {
	priority = p_priority;
}

int32_t AICompletionRequestResource::get_priority() const {
	return priority;
}

void AICompletionRequestResource::set_stream(bool p_stream) {
	stream = p_stream;
}

bool AICompletionRequestResource::is_streaming() const {
	return stream;
}

AI_REQUEST_RESOURCE_REF_ACCESSORS(AIEmbeddingRequestResource, PackedStringArray, inputs)
AI_REQUEST_RESOURCE_REF_ACCESSORS(AIEmbeddingRequestResource, String, caller_tag)
AI_REQUEST_RESOURCE_REF_ACCESSORS(AIEmbeddingRequestResource, Dictionary, metadata)

void AIEmbeddingRequestResource::set_timeout_ms(int32_t p_timeout_ms) {
	timeout_ms = p_timeout_ms;
}

int32_t AIEmbeddingRequestResource::get_timeout_ms() const {
	return timeout_ms;
}

void AIEmbeddingRequestResource::set_priority(int32_t p_priority) {
	priority = p_priority;
}

int32_t AIEmbeddingRequestResource::get_priority() const {
	return priority;
}

void AIEmbeddingRequestResource::set_normalize(bool p_normalize) {
	normalize = p_normalize;
}

bool AIEmbeddingRequestResource::is_normalized() const {
	return normalize;
}
