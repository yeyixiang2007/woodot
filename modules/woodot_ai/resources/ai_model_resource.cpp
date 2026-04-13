/**************************************************************************/
/*  ai_model_resource.cpp                                                 */
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

#include "modules/woodot_ai/resources/ai_model_resource.h"

#include "core/object/class_db.h"

void AIModelResource::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_backend_name", "backend_name"), &AIModelResource::set_backend_name);
	ClassDB::bind_method(D_METHOD("get_backend_name"), &AIModelResource::get_backend_name);
	ClassDB::bind_method(D_METHOD("set_source_path", "source_path"), &AIModelResource::set_source_path);
	ClassDB::bind_method(D_METHOD("get_source_path"), &AIModelResource::get_source_path);
	ClassDB::bind_method(D_METHOD("set_backend_options", "backend_options"), &AIModelResource::set_backend_options);
	ClassDB::bind_method(D_METHOD("get_backend_options"), &AIModelResource::get_backend_options);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "backend_name"), "set_backend_name", "get_backend_name");
	ADD_PROPERTY_DEFAULT("backend_name", StringName("llama"));
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "source_path"), "set_source_path", "get_source_path");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "backend_options"), "set_backend_options", "get_backend_options");
}

void AIModelResource::set_backend_name(const StringName &p_backend_name) {
	backend_name = p_backend_name;
}

StringName AIModelResource::get_backend_name() const {
	return backend_name;
}

void AIModelResource::set_source_path(const String &p_source_path) {
	source_path = p_source_path;
}

String AIModelResource::get_source_path() const {
	return source_path;
}

void AIModelResource::set_backend_options(const Dictionary &p_backend_options) {
	backend_options = p_backend_options;
}

Dictionary AIModelResource::get_backend_options() const {
	return backend_options;
}
