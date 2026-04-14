/**************************************************************************/
/*  gdscript_repair_engine.cpp                                            */
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

#include "modules/woodot_ai/editor/gdscript_repair_engine.h"

#include "core/io/json.h"
#include "core/object/class_db.h"

GDScriptRepairEngine *GDScriptRepairEngine::singleton = nullptr;

void GDScriptRepairEngine::_bind_methods() {
	ClassDB::bind_method(D_METHOD("validate_patch_ir", "source_ir"), &GDScriptRepairEngine::validate_patch_ir);
	ClassDB::bind_method(D_METHOD("parse_patch_ir", "source_ir", "script_path", "diagnostic_message", "metadata"), &GDScriptRepairEngine::parse_patch_ir, DEFVAL(String()), DEFVAL(String()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("get_engine_status"), &GDScriptRepairEngine::get_engine_status);
}

GDScriptRepairEngine *GDScriptRepairEngine::get_singleton() {
	return singleton;
}

Dictionary GDScriptRepairEngine::validate_patch_ir(const String &p_source_ir) const {
	Dictionary result;
	Array errors;
	Array warnings;
	Array validated_hunks;

	result["valid"] = false;
	result["format"] = "gdscript_patch";
	result["engine"] = "GDScriptRepairEngine";

	if (p_source_ir.strip_edges().is_empty()) {
		errors.push_back("Patch IR must not be empty.");
		result["errors"] = errors;
		result["warnings"] = warnings;
		return result;
	}

	Ref<JSON> json;
	json.instantiate();
	const Error parse_error = json->parse(p_source_ir);
	if (parse_error != OK) {
		errors.push_back(vformat("JSON parse failed at line %d: %s", json->get_error_line(), json->get_error_message()));
		result["errors"] = errors;
		result["warnings"] = warnings;
		return result;
	}

	const Variant data = json->get_data();
	if (data.get_type() != Variant::DICTIONARY) {
		errors.push_back("Patch IR root must be a JSON object.");
		result["errors"] = errors;
		result["warnings"] = warnings;
		return result;
	}

	const Dictionary root = data;
	if (root.has("script_path") && root["script_path"].get_type() != Variant::STRING) {
		errors.push_back("Field 'script_path' must be a string.");
	}
	if (root.has("diagnostic_message") && root["diagnostic_message"].get_type() != Variant::STRING) {
		errors.push_back("Field 'diagnostic_message' must be a string.");
	}
	if (root.has("line_start") && root["line_start"].get_type() != Variant::INT) {
		errors.push_back("Field 'line_start' must be an integer.");
	}
	if (root.has("line_end") && root["line_end"].get_type() != Variant::INT) {
		errors.push_back("Field 'line_end' must be an integer.");
	}
	if (root.has("replacement_text") && root["replacement_text"].get_type() != Variant::STRING) {
		errors.push_back("Field 'replacement_text' must be a string.");
	}
	if (root.has("warnings") && root["warnings"].get_type() != Variant::ARRAY) {
		errors.push_back("Field 'warnings' must be an array.");
	}
	if (root.has("metadata") && root["metadata"].get_type() != Variant::DICTIONARY) {
		errors.push_back("Field 'metadata' must be a dictionary.");
	}
	if (root.has("hunks") && root["hunks"].get_type() != Variant::ARRAY) {
		errors.push_back("Field 'hunks' must be an array.");
	}

	if (root.has("line_start") && root.has("line_end") && int32_t(root["line_start"]) > int32_t(root["line_end"])) {
		errors.push_back("Field 'line_start' must be less than or equal to 'line_end'.");
	}

	if (errors.is_empty() && root.has("hunks")) {
		const Array hunks = root["hunks"];
		for (int32_t i = 0; i < hunks.size(); i++) {
			if (hunks[i].get_type() != Variant::DICTIONARY) {
				errors.push_back(vformat("hunks[%d] must be an object.", i));
				continue;
			}

			const Dictionary validation = _validate_hunk(hunks[i], i);
			const Array hunk_errors = validation["errors"];
			const Array hunk_warnings = validation["warnings"];
			for (int32_t error_index = 0; error_index < hunk_errors.size(); error_index++) {
				errors.push_back(hunk_errors[error_index]);
			}
			for (int32_t warning_index = 0; warning_index < hunk_warnings.size(); warning_index++) {
				warnings.push_back(hunk_warnings[warning_index]);
			}
			validated_hunks.push_back(validation["normalized"]);
		}
	}

	if (root.has("warnings") && root["warnings"].get_type() == Variant::ARRAY) {
		const Array ir_warnings = _normalize_string_array(root["warnings"]);
		for (int32_t i = 0; i < ir_warnings.size(); i++) {
			warnings.push_back(ir_warnings[i]);
		}
	}

	result["valid"] = errors.is_empty();
	result["errors"] = errors;
	result["warnings"] = warnings;
	result["hunks"] = validated_hunks;
	result["script_path"] = root.has("script_path") ? String(root["script_path"]) : String();
	result["diagnostic_message"] = root.has("diagnostic_message") ? String(root["diagnostic_message"]) : String();
	result["line_start"] = root.has("line_start") ? int32_t(root["line_start"]) : 0;
	result["line_end"] = root.has("line_end") ? int32_t(root["line_end"]) : 0;
	result["replacement_text"] = root.has("replacement_text") ? String(root["replacement_text"]) : String();
	if (root.has("metadata") && root["metadata"].get_type() == Variant::DICTIONARY) {
		result["metadata"] = Dictionary(root["metadata"]);
	} else {
		result["metadata"] = Dictionary();
	}
	return result;
}

Ref<GDScriptRepairPatch> GDScriptRepairEngine::parse_patch_ir(const String &p_source_ir, const String &p_script_path, const String &p_diagnostic_message, const Dictionary &p_metadata) const {
	const Dictionary validation = validate_patch_ir(p_source_ir);
	const bool valid = validation.has("valid") ? bool(validation["valid"]) : false;
	ERR_FAIL_COND_V_MSG(!valid, Ref<GDScriptRepairPatch>(), "GDScriptRepairEngine rejected the provided patch IR.");

	Ref<GDScriptRepairPatch> patch;
	patch.instantiate();
	patch->set_script_path(!p_script_path.is_empty() ? p_script_path : String(validation["script_path"]));
	patch->set_diagnostic_message(!p_diagnostic_message.is_empty() ? p_diagnostic_message : String(validation["diagnostic_message"]));
	patch->set_line_start(validation["line_start"]);
	patch->set_line_end(validation["line_end"]);
	patch->set_replacement_text(validation["replacement_text"]);
	patch->set_hunks(validation["hunks"]);
	patch->set_warnings(validation["warnings"]);

	Dictionary metadata = validation["metadata"];
	const Array override_keys = p_metadata.keys();
	for (int32_t i = 0; i < override_keys.size(); i++) {
		metadata[override_keys[i]] = p_metadata[override_keys[i]];
	}
	metadata["engine"] = "GDScriptRepairEngine";
	metadata["format"] = "gdscript_patch";
	patch->set_metadata(metadata);
	return patch;
}

Dictionary GDScriptRepairEngine::get_engine_status() const {
	Dictionary status;
	Array allowed_operations;
	allowed_operations.push_back("replace_range");
	allowed_operations.push_back("insert_after");
	allowed_operations.push_back("insert_before");

	status["format"] = "gdscript_patch";
	status["allowed_hunk_operations"] = allowed_operations;
	status["requires_json_object_root"] = true;
	status["supports_multi_hunk"] = true;
	return status;
}

Dictionary GDScriptRepairEngine::_validate_hunk(const Dictionary &p_hunk, int32_t p_index) const {
	Dictionary result;
	Array errors;
	Array warnings;
	Dictionary normalized = p_hunk;

	if (!p_hunk.has("op")) {
		errors.push_back(vformat("hunks[%d] is missing required field 'op'.", p_index));
	} else if (p_hunk["op"].get_type() != Variant::STRING) {
		errors.push_back(vformat("hunks[%d].op must be a string.", p_index));
	} else if (!_is_allowed_patch_operation(p_hunk["op"])) {
		errors.push_back(vformat("hunks[%d].op '%s' is not supported.", p_index, String(p_hunk["op"])));
	}

	if (!p_hunk.has("line_start")) {
		errors.push_back(vformat("hunks[%d] is missing required field 'line_start'.", p_index));
	} else if (p_hunk["line_start"].get_type() != Variant::INT) {
		errors.push_back(vformat("hunks[%d].line_start must be an integer.", p_index));
	}

	if (!p_hunk.has("line_end")) {
		errors.push_back(vformat("hunks[%d] is missing required field 'line_end'.", p_index));
	} else if (p_hunk["line_end"].get_type() != Variant::INT) {
		errors.push_back(vformat("hunks[%d].line_end must be an integer.", p_index));
	}

	if (!p_hunk.has("replacement_text")) {
		errors.push_back(vformat("hunks[%d] is missing required field 'replacement_text'.", p_index));
	} else if (p_hunk["replacement_text"].get_type() != Variant::STRING) {
		errors.push_back(vformat("hunks[%d].replacement_text must be a string.", p_index));
	}

	if (p_hunk.has("line_start") && p_hunk.has("line_end") && p_hunk["line_start"].get_type() == Variant::INT && p_hunk["line_end"].get_type() == Variant::INT) {
		if (int32_t(p_hunk["line_start"]) > int32_t(p_hunk["line_end"])) {
			errors.push_back(vformat("hunks[%d].line_start must be less than or equal to line_end.", p_index));
		}
	}

	if (p_hunk.has("context") && p_hunk["context"].get_type() != Variant::STRING) {
		errors.push_back(vformat("hunks[%d].context must be a string when provided.", p_index));
	}

	result["errors"] = errors;
	result["warnings"] = warnings;
	result["normalized"] = normalized;
	return result;
}

Array GDScriptRepairEngine::_normalize_string_array(const Variant &p_value) const {
	Array normalized;
	if (p_value.get_type() != Variant::ARRAY) {
		return normalized;
	}

	const Array source = p_value;
	for (int32_t i = 0; i < source.size(); i++) {
		normalized.push_back(String(source[i]));
	}
	return normalized;
}

bool GDScriptRepairEngine::_is_allowed_patch_operation(const String &p_operation) const {
	return p_operation == "replace_range" || p_operation == "insert_after" || p_operation == "insert_before";
}

GDScriptRepairEngine::GDScriptRepairEngine() {
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;
}

GDScriptRepairEngine::~GDScriptRepairEngine() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
