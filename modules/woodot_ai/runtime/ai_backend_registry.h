/**************************************************************************/
/*  ai_backend_registry.h                                                 */
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

#include "modules/woodot_ai/runtime/ai_backend.h"

#include "core/templates/hash_map.h"
#include "core/variant/variant.h"

class AIBackendRegistry {
	HashMap<StringName, AIBackend *> backends;

public:
	void register_backend(const StringName &p_name, AIBackend *p_backend) {
		backends.insert(p_name, p_backend);
	}

	void unregister_backend(const StringName &p_name) {
		backends.erase(p_name);
	}

	bool has_backend(const StringName &p_name) const {
		return backends.has(p_name);
	}

	AIBackend *get_backend(const StringName &p_name) {
		HashMap<StringName, AIBackend *>::Iterator backend = backends.find(p_name);
		return backend ? backend->value : nullptr;
	}

	const AIBackend *get_backend(const StringName &p_name) const {
		HashMap<StringName, AIBackend *>::ConstIterator backend = backends.find(p_name);
		return backend ? backend->value : nullptr;
	}

	PackedStringArray get_backend_names() const {
		PackedStringArray names;
		for (const KeyValue<StringName, AIBackend *> &E : backends) {
			names.push_back(String(E.key));
		}
		return names;
	}
};
