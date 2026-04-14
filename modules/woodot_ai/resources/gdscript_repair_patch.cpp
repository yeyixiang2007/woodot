/**************************************************************************/
/*  gdscript_repair_patch.cpp                                             */
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

#include "modules/woodot_ai/resources/gdscript_repair_patch.h"

#include "core/object/class_db.h"

void GDScriptRepairPatch::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_script_path", "script_path"), &GDScriptRepairPatch::set_script_path);
	ClassDB::bind_method(D_METHOD("get_script_path"), &GDScriptRepairPatch::get_script_path);
	ClassDB::bind_method(D_METHOD("set_diagnostic_message", "diagnostic_message"), &GDScriptRepairPatch::set_diagnostic_message);
	ClassDB::bind_method(D_METHOD("get_diagnostic_message"), &GDScriptRepairPatch::get_diagnostic_message);
	ClassDB::bind_method(D_METHOD("set_line_start", "line_start"), &GDScriptRepairPatch::set_line_start);
	ClassDB::bind_method(D_METHOD("get_line_start"), &GDScriptRepairPatch::get_line_start);
	ClassDB::bind_method(D_METHOD("set_line_end", "line_end"), &GDScriptRepairPatch::set_line_end);
	ClassDB::bind_method(D_METHOD("get_line_end"), &GDScriptRepairPatch::get_line_end);
	ClassDB::bind_method(D_METHOD("set_replacement_text", "replacement_text"), &GDScriptRepairPatch::set_replacement_text);
	ClassDB::bind_method(D_METHOD("get_replacement_text"), &GDScriptRepairPatch::get_replacement_text);
	ClassDB::bind_method(D_METHOD("set_hunks", "hunks"), &GDScriptRepairPatch::set_hunks);
	ClassDB::bind_method(D_METHOD("get_hunks"), &GDScriptRepairPatch::get_hunks);
	ClassDB::bind_method(D_METHOD("set_warnings", "warnings"), &GDScriptRepairPatch::set_warnings);
	ClassDB::bind_method(D_METHOD("get_warnings"), &GDScriptRepairPatch::get_warnings);
	ClassDB::bind_method(D_METHOD("set_metadata", "metadata"), &GDScriptRepairPatch::set_metadata);
	ClassDB::bind_method(D_METHOD("get_metadata"), &GDScriptRepairPatch::get_metadata);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "script_path"), "set_script_path", "get_script_path");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "diagnostic_message", PROPERTY_HINT_MULTILINE_TEXT), "set_diagnostic_message", "get_diagnostic_message");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "line_start"), "set_line_start", "get_line_start");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "line_end"), "set_line_end", "get_line_end");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "replacement_text", PROPERTY_HINT_MULTILINE_TEXT), "set_replacement_text", "get_replacement_text");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "hunks"), "set_hunks", "get_hunks");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "warnings"), "set_warnings", "get_warnings");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "metadata"), "set_metadata", "get_metadata");
}

#define GDSCRIPT_REPAIR_PATCH_REF_ACCESSORS(m_type, m_name) \
	void GDScriptRepairPatch::set_##m_name(const m_type &p_##m_name) { m_name = p_##m_name; } \
	m_type GDScriptRepairPatch::get_##m_name() const { return m_name; }

GDSCRIPT_REPAIR_PATCH_REF_ACCESSORS(String, script_path)
GDSCRIPT_REPAIR_PATCH_REF_ACCESSORS(String, diagnostic_message)
GDSCRIPT_REPAIR_PATCH_REF_ACCESSORS(String, replacement_text)
GDSCRIPT_REPAIR_PATCH_REF_ACCESSORS(Array, hunks)
GDSCRIPT_REPAIR_PATCH_REF_ACCESSORS(Array, warnings)
GDSCRIPT_REPAIR_PATCH_REF_ACCESSORS(Dictionary, metadata)

void GDScriptRepairPatch::set_line_start(int32_t p_line_start) {
	line_start = p_line_start;
}

int32_t GDScriptRepairPatch::get_line_start() const {
	return line_start;
}

void GDScriptRepairPatch::set_line_end(int32_t p_line_end) {
	line_end = p_line_end;
}

int32_t GDScriptRepairPatch::get_line_end() const {
	return line_end;
}
