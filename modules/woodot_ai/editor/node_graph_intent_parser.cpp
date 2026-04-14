/**************************************************************************/
/*  node_graph_intent_parser.cpp                                          */
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

#include "modules/woodot_ai/editor/node_graph_intent_parser.h"

#include "core/io/json.h"
#include "core/object/class_db.h"

NodeGraphIntentParser *NodeGraphIntentParser::singleton = nullptr;

void NodeGraphIntentParser::_bind_methods() {
	ClassDB::bind_method(D_METHOD("validate_scene_plan_ir", "source_ir"), &NodeGraphIntentParser::validate_scene_plan_ir);
	ClassDB::bind_method(D_METHOD("parse_scene_plan_ir", "source_ir", "prompt", "metadata"), &NodeGraphIntentParser::parse_scene_plan_ir, DEFVAL(String()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("get_parser_status"), &NodeGraphIntentParser::get_parser_status);
}

NodeGraphIntentParser *NodeGraphIntentParser::get_singleton() {
	return singleton;
}

Dictionary NodeGraphIntentParser::validate_scene_plan_ir(const String &p_source_ir) const {
	Dictionary result;
	Array errors;
	Array warnings;
	Array validated_node_operations;
	Array validated_resource_operations;

	result["valid"] = false;
	result["format"] = "scene_plan_ir";
	result["parser"] = "NodeGraphIntentParser";

	if (p_source_ir.strip_edges().is_empty()) {
		errors.push_back("Scene plan IR must not be empty.");
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
		errors.push_back("Scene plan IR root must be a JSON object.");
		result["errors"] = errors;
		result["warnings"] = warnings;
		return result;
	}

	const Dictionary root = data;
	if (!root.has("node_operations")) {
		errors.push_back("Scene plan IR is missing required field 'node_operations'.");
	}
	if (root.has("node_operations") && root["node_operations"].get_type() != Variant::ARRAY) {
		errors.push_back("Field 'node_operations' must be an array.");
	}
	if (root.has("resource_operations") && root["resource_operations"].get_type() != Variant::ARRAY) {
		errors.push_back("Field 'resource_operations' must be an array.");
	}
	if (root.has("warnings") && root["warnings"].get_type() != Variant::ARRAY) {
		errors.push_back("Field 'warnings' must be an array.");
	}
	if (root.has("metadata") && root["metadata"].get_type() != Variant::DICTIONARY) {
		errors.push_back("Field 'metadata' must be a dictionary.");
	}

	if (errors.is_empty() && root.has("node_operations")) {
		const Array node_operations = root["node_operations"];
		for (int32_t i = 0; i < node_operations.size(); i++) {
			if (node_operations[i].get_type() != Variant::DICTIONARY) {
				errors.push_back(vformat("node_operations[%d] must be an object.", i));
				continue;
			}

			const Dictionary validation = _validate_node_operation(node_operations[i], i);
			const Array operation_errors = validation["errors"];
			const Array operation_warnings = validation["warnings"];
			for (int32_t error_index = 0; error_index < operation_errors.size(); error_index++) {
				errors.push_back(operation_errors[error_index]);
			}
			for (int32_t warning_index = 0; warning_index < operation_warnings.size(); warning_index++) {
				warnings.push_back(operation_warnings[warning_index]);
			}
			validated_node_operations.push_back(validation["normalized"]);
		}
	}

	if (errors.is_empty() && root.has("resource_operations")) {
		const Array resource_operations = root["resource_operations"];
		for (int32_t i = 0; i < resource_operations.size(); i++) {
			if (resource_operations[i].get_type() != Variant::DICTIONARY) {
				errors.push_back(vformat("resource_operations[%d] must be an object.", i));
				continue;
			}

			const Dictionary validation = _validate_resource_operation(resource_operations[i], i);
			const Array operation_errors = validation["errors"];
			const Array operation_warnings = validation["warnings"];
			for (int32_t error_index = 0; error_index < operation_errors.size(); error_index++) {
				errors.push_back(operation_errors[error_index]);
			}
			for (int32_t warning_index = 0; warning_index < operation_warnings.size(); warning_index++) {
				warnings.push_back(operation_warnings[warning_index]);
			}
			validated_resource_operations.push_back(validation["normalized"]);
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
	result["node_operations"] = validated_node_operations;
	result["resource_operations"] = validated_resource_operations;
	if (root.has("metadata") && root["metadata"].get_type() == Variant::DICTIONARY) {
		result["metadata"] = Dictionary(root["metadata"]);
	} else {
		result["metadata"] = Dictionary();
	}
	result["prompt"] = root.has("prompt") ? String(root["prompt"]) : String();
	return result;
}

Ref<SceneSynthesisPlan> NodeGraphIntentParser::parse_scene_plan_ir(const String &p_source_ir, const String &p_prompt, const Dictionary &p_metadata) const {
	const Dictionary validation = validate_scene_plan_ir(p_source_ir);
	const bool valid = validation.has("valid") ? bool(validation["valid"]) : false;
	ERR_FAIL_COND_V_MSG(!valid, Ref<SceneSynthesisPlan>(), "NodeGraphIntentParser rejected the provided scene plan IR.");

	Ref<SceneSynthesisPlan> plan;
	plan.instantiate();
	plan->set_source_ir(p_source_ir);
	plan->set_prompt(!p_prompt.is_empty() ? p_prompt : String(validation["prompt"]));
	plan->set_node_operations(validation["node_operations"]);
	plan->set_resource_operations(validation["resource_operations"]);
	plan->set_warnings(validation["warnings"]);

	Dictionary plan_metadata = validation["metadata"];
	const Array override_keys = p_metadata.keys();
	for (int32_t i = 0; i < override_keys.size(); i++) {
		plan_metadata[override_keys[i]] = p_metadata[override_keys[i]];
	}
	plan_metadata["parser"] = "NodeGraphIntentParser";
	plan_metadata["format"] = "scene_plan_ir";
	plan->set_metadata(plan_metadata);
	return plan;
}

Dictionary NodeGraphIntentParser::get_parser_status() const {
	Dictionary status;
	Array allowed_node_operations;
	allowed_node_operations.push_back("create_root");
	allowed_node_operations.push_back("create_node");
	allowed_node_operations.push_back("set_property");
	allowed_node_operations.push_back("remove_node");
	allowed_node_operations.push_back("reparent_node");

	Array allowed_resource_operations;
	allowed_resource_operations.push_back("load_resource");
	allowed_resource_operations.push_back("create_resource");
	allowed_resource_operations.push_back("assign_resource");

	status["format"] = "scene_plan_ir";
	status["allowed_node_operations"] = allowed_node_operations;
	status["allowed_resource_operations"] = allowed_resource_operations;
	status["requires_json_object_root"] = true;
	status["validates_node_types"] = true;
	return status;
}

Dictionary NodeGraphIntentParser::_validate_node_operation(const Dictionary &p_operation, int32_t p_index) const {
	Dictionary result;
	Array errors;
	Array warnings;
	Dictionary normalized = p_operation;

	if (!p_operation.has("op")) {
		errors.push_back(vformat("node_operations[%d] is missing required field 'op'.", p_index));
	} else if (p_operation["op"].get_type() != Variant::STRING) {
		errors.push_back(vformat("node_operations[%d].op must be a string.", p_index));
	} else {
		const String operation = p_operation["op"];
		if (!_is_allowed_node_operation(operation)) {
			errors.push_back(vformat("node_operations[%d].op '%s' is not supported.", p_index, operation));
		}
	}

	if (p_operation.has("node_type")) {
		if (p_operation["node_type"].get_type() != Variant::STRING) {
			errors.push_back(vformat("node_operations[%d].node_type must be a string.", p_index));
		} else {
			const StringName node_type = String(p_operation["node_type"]);
			if (!ClassDB::class_exists(node_type)) {
				errors.push_back(vformat("node_operations[%d].node_type '%s' does not exist.", p_index, String(node_type)));
			} else if (!ClassDB::is_parent_class(node_type, "Node")) {
				errors.push_back(vformat("node_operations[%d].node_type '%s' is not a Node type.", p_index, String(node_type)));
			} else if (!ClassDB::can_instantiate(node_type)) {
				warnings.push_back(vformat("node_operations[%d].node_type '%s' cannot be instantiated directly.", p_index, String(node_type)));
			}
		}
	}

	if (p_operation.has("target_path") && p_operation["target_path"].get_type() != Variant::STRING) {
		errors.push_back(vformat("node_operations[%d].target_path must be a string.", p_index));
	}
	if (p_operation.has("parent_path") && p_operation["parent_path"].get_type() != Variant::STRING) {
		errors.push_back(vformat("node_operations[%d].parent_path must be a string.", p_index));
	}
	if (p_operation.has("name") && p_operation["name"].get_type() != Variant::STRING) {
		errors.push_back(vformat("node_operations[%d].name must be a string.", p_index));
	}
	if (p_operation.has("property") && p_operation["property"].get_type() != Variant::STRING) {
		errors.push_back(vformat("node_operations[%d].property must be a string.", p_index));
	}
	if (p_operation.has("children") && p_operation["children"].get_type() != Variant::ARRAY) {
		errors.push_back(vformat("node_operations[%d].children must be an array when provided.", p_index));
	}

	result["errors"] = errors;
	result["warnings"] = warnings;
	result["normalized"] = normalized;
	return result;
}

Dictionary NodeGraphIntentParser::_validate_resource_operation(const Dictionary &p_operation, int32_t p_index) const {
	Dictionary result;
	Array errors;
	Array warnings;
	Dictionary normalized = p_operation;

	if (!p_operation.has("op")) {
		errors.push_back(vformat("resource_operations[%d] is missing required field 'op'.", p_index));
	} else if (p_operation["op"].get_type() != Variant::STRING) {
		errors.push_back(vformat("resource_operations[%d].op must be a string.", p_index));
	} else if (!_is_allowed_resource_operation(p_operation["op"])) {
		errors.push_back(vformat("resource_operations[%d].op '%s' is not supported.", p_index, String(p_operation["op"])));
	}

	if (p_operation.has("path") && p_operation["path"].get_type() != Variant::STRING) {
		errors.push_back(vformat("resource_operations[%d].path must be a string.", p_index));
	}
	if (p_operation.has("resource_type") && p_operation["resource_type"].get_type() != Variant::STRING) {
		errors.push_back(vformat("resource_operations[%d].resource_type must be a string.", p_index));
	}
	if (p_operation.has("assign_to") && p_operation["assign_to"].get_type() != Variant::STRING) {
		errors.push_back(vformat("resource_operations[%d].assign_to must be a string.", p_index));
	}

	result["errors"] = errors;
	result["warnings"] = warnings;
	result["normalized"] = normalized;
	return result;
}

Array NodeGraphIntentParser::_normalize_string_array(const Variant &p_value) const {
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

bool NodeGraphIntentParser::_is_allowed_node_operation(const String &p_operation) const {
	return p_operation == "create_root" || p_operation == "create_node" || p_operation == "set_property" || p_operation == "remove_node" || p_operation == "reparent_node";
}

bool NodeGraphIntentParser::_is_allowed_resource_operation(const String &p_operation) const {
	return p_operation == "load_resource" || p_operation == "create_resource" || p_operation == "assign_resource";
}

NodeGraphIntentParser::NodeGraphIntentParser() {
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;
}

NodeGraphIntentParser::~NodeGraphIntentParser() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
