/**************************************************************************/
/*  editor_context_collector.h                                            */
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
#include "core/variant/type_info.h"

class Node;

class EditorContextCollector : public Object {
	GDCLASS(EditorContextCollector, Object);

public:
	struct ContextBudget {
		int32_t scene_node_budget = 0;
		int32_t scene_depth_budget = 0;
		int32_t selection_budget = 0;
		int32_t text_preview_budget = 0;
	};

	enum BudgetProfile {
		BUDGET_PROFILE_SCENE_SYNTHESIS = 0,
		BUDGET_PROFILE_SCRIPT_REPAIR,
	};

private:
	static EditorContextCollector *singleton;
	static constexpr int32_t MIN_SCENE_NODE_BUDGET = 0;
	static constexpr int32_t MAX_SCENE_NODE_BUDGET = 512;
	static constexpr int32_t MIN_SCENE_DEPTH_BUDGET = 0;
	static constexpr int32_t MAX_SCENE_DEPTH_BUDGET = 8;
	static constexpr int32_t MIN_SELECTION_BUDGET = 0;
	static constexpr int32_t MAX_SELECTION_BUDGET = 64;
	static constexpr int32_t MIN_TEXT_PREVIEW_BUDGET = 0;
	static constexpr int32_t MAX_TEXT_PREVIEW_BUDGET = 16000;

	ContextBudget scene_synthesis_budget;
	ContextBudget script_repair_budget;

	ContextBudget _sanitize_budget(const Dictionary &p_budget, const ContextBudget &p_fallback) const;
	Dictionary _serialize_budget(const ContextBudget &p_budget) const;
	Dictionary _build_scene_context(const ContextBudget &p_budget) const;
	Dictionary _build_script_context(const String &p_script_path, const String &p_diagnostics, const String &p_code_snippet, const ContextBudget &p_budget) const;
	Dictionary _build_project_context() const;
	Dictionary _build_budget_info() const;
	Dictionary _snapshot_node(Node *p_node, int32_t p_depth, int32_t &r_remaining_budget, const ContextBudget &p_budget) const;
	Array _snapshot_selection(const ContextBudget &p_budget) const;
	String _load_script_snippet(const String &p_script_path) const;
	String _truncate_text(const String &p_text, int32_t p_limit) const;
	Dictionary _merge_context(const Dictionary &p_base, const Dictionary &p_overrides) const;

protected:
	static void _bind_methods();

public:
	static EditorContextCollector *get_singleton();

	bool has_editor_interface() const;
	void set_scene_synthesis_budget(const Dictionary &p_budget);
	Dictionary get_scene_synthesis_budget() const;
	void set_script_repair_budget(const Dictionary &p_budget);
	Dictionary get_script_repair_budget() const;
	Dictionary collect_scene_request_context(const Dictionary &p_overrides = Dictionary()) const;
	Dictionary collect_script_repair_context(const String &p_script_path, const String &p_diagnostics, const String &p_code_snippet = String(), const Dictionary &p_overrides = Dictionary()) const;
	Dictionary get_collector_status() const;

	EditorContextCollector();
	~EditorContextCollector();
};

VARIANT_ENUM_CAST(EditorContextCollector::BudgetProfile);
