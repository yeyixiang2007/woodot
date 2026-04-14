/**************************************************************************/
/*  undo_redo_bridge.h                                                    */
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

class GDScriptRepairPatch;
class Node;
class SceneSynthesisPlan;

class UndoRedoBridge : public Object {
	GDCLASS(UndoRedoBridge, Object);

	static UndoRedoBridge *singleton;

	Error _read_text_file(const String &p_path, String &r_contents) const;
	Array _split_text_lines(const String &p_text, bool *r_has_trailing_newline = nullptr) const;
	String _join_text_lines(const Array &p_lines, bool p_has_trailing_newline) const;
	Error _apply_patch_hunks_to_text(const String &p_original_text, const Array &p_hunks, String &r_output_text, String &r_error_message) const;
	Dictionary _make_status(Error p_error, const String &p_message, bool p_can_apply) const;
	Node *_get_edited_scene_root() const;
	Node *_resolve_scene_node(const String &p_path) const;
	String _join_scene_path(const String &p_parent_path, const String &p_name) const;

protected:
	static void _bind_methods();

public:
	static UndoRedoBridge *get_singleton();

	void _attach_scene_node(const String &p_parent_path, Node *p_node);
	void _detach_scene_node(const String &p_parent_path, Node *p_node);
	void _set_scene_node_owner(Node *p_node);
	void _write_text_file(const String &p_path, const String &p_contents);
	Dictionary can_apply_scene_plan(const Ref<SceneSynthesisPlan> &p_plan) const;
	Dictionary apply_scene_plan(const Ref<SceneSynthesisPlan> &p_plan);
	Dictionary can_apply_gdscript_patch(const Ref<GDScriptRepairPatch> &p_patch) const;
	Dictionary apply_gdscript_patch(const Ref<GDScriptRepairPatch> &p_patch);
	Dictionary get_bridge_status() const;

	UndoRedoBridge();
	~UndoRedoBridge();
};
