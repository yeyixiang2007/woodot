/**************************************************************************/
/*  scene_synthesis_plan.cpp                                              */
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

#include "modules/woodot_ai/resources/scene_synthesis_plan.h"

#include "core/object/class_db.h"

void SceneSynthesisPlan::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_prompt", "prompt"), &SceneSynthesisPlan::set_prompt);
	ClassDB::bind_method(D_METHOD("get_prompt"), &SceneSynthesisPlan::get_prompt);
	ClassDB::bind_method(D_METHOD("set_source_ir", "source_ir"), &SceneSynthesisPlan::set_source_ir);
	ClassDB::bind_method(D_METHOD("get_source_ir"), &SceneSynthesisPlan::get_source_ir);
	ClassDB::bind_method(D_METHOD("set_node_operations", "node_operations"), &SceneSynthesisPlan::set_node_operations);
	ClassDB::bind_method(D_METHOD("get_node_operations"), &SceneSynthesisPlan::get_node_operations);
	ClassDB::bind_method(D_METHOD("set_resource_operations", "resource_operations"), &SceneSynthesisPlan::set_resource_operations);
	ClassDB::bind_method(D_METHOD("get_resource_operations"), &SceneSynthesisPlan::get_resource_operations);
	ClassDB::bind_method(D_METHOD("set_warnings", "warnings"), &SceneSynthesisPlan::set_warnings);
	ClassDB::bind_method(D_METHOD("get_warnings"), &SceneSynthesisPlan::get_warnings);
	ClassDB::bind_method(D_METHOD("set_metadata", "metadata"), &SceneSynthesisPlan::set_metadata);
	ClassDB::bind_method(D_METHOD("get_metadata"), &SceneSynthesisPlan::get_metadata);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "prompt", PROPERTY_HINT_MULTILINE_TEXT), "set_prompt", "get_prompt");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "source_ir", PROPERTY_HINT_MULTILINE_TEXT), "set_source_ir", "get_source_ir");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "node_operations"), "set_node_operations", "get_node_operations");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "resource_operations"), "set_resource_operations", "get_resource_operations");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "warnings"), "set_warnings", "get_warnings");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "metadata"), "set_metadata", "get_metadata");
}

#define SCENE_SYNTHESIS_PLAN_REF_ACCESSORS(m_type, m_name) \
	void SceneSynthesisPlan::set_##m_name(const m_type &p_##m_name) { m_name = p_##m_name; } \
	m_type SceneSynthesisPlan::get_##m_name() const { return m_name; }

SCENE_SYNTHESIS_PLAN_REF_ACCESSORS(String, prompt)
SCENE_SYNTHESIS_PLAN_REF_ACCESSORS(String, source_ir)
SCENE_SYNTHESIS_PLAN_REF_ACCESSORS(Array, node_operations)
SCENE_SYNTHESIS_PLAN_REF_ACCESSORS(Array, resource_operations)
SCENE_SYNTHESIS_PLAN_REF_ACCESSORS(Array, warnings)
SCENE_SYNTHESIS_PLAN_REF_ACCESSORS(Dictionary, metadata)
