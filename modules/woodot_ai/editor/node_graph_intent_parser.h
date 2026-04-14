/**************************************************************************/
/*  node_graph_intent_parser.h                                            */
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

#include "core/object/object.h"
#include "modules/woodot_ai/resources/scene_synthesis_plan.h"

class NodeGraphIntentParser : public Object {
	GDCLASS(NodeGraphIntentParser, Object);

	static NodeGraphIntentParser *singleton;

	Dictionary _validate_node_operation(const Dictionary &p_operation, int32_t p_index) const;
	Dictionary _validate_resource_operation(const Dictionary &p_operation, int32_t p_index) const;
	Array _normalize_string_array(const Variant &p_value) const;
	bool _is_allowed_node_operation(const String &p_operation) const;
	bool _is_allowed_resource_operation(const String &p_operation) const;

protected:
	static void _bind_methods();

public:
	static NodeGraphIntentParser *get_singleton();

	Dictionary validate_scene_plan_ir(const String &p_source_ir) const;
	Ref<SceneSynthesisPlan> parse_scene_plan_ir(const String &p_source_ir, const String &p_prompt = String(), const Dictionary &p_metadata = Dictionary()) const;
	Dictionary get_parser_status() const;

	NodeGraphIntentParser();
	~NodeGraphIntentParser();
};
