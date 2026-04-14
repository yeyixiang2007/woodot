/**************************************************************************/
/*  ai_mesh_post_processor.cpp                                            */
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

#include "modules/woodot_ai/import/ai_mesh_post_processor.h"

#include "core/object/class_db.h"
#include "modules/woodot_ai/import/ai_import_orchestrator.h"

AIMeshPostProcessor *AIMeshPostProcessor::singleton = nullptr;

void AIMeshPostProcessor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("has_import_orchestrator"), &AIMeshPostProcessor::has_import_orchestrator);
	ClassDB::bind_method(D_METHOD("is_ready"), &AIMeshPostProcessor::is_ready);
	ClassDB::bind_method(D_METHOD("describe_processing_scope", "source_path", "importer_name", "options"), &AIMeshPostProcessor::describe_processing_scope, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("build_processing_plan", "source_path", "importer_name", "options"), &AIMeshPostProcessor::build_processing_plan, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("build_review_report", "plan"), &AIMeshPostProcessor::build_review_report);
	ClassDB::bind_method(D_METHOD("get_processor_status"), &AIMeshPostProcessor::get_processor_status);

	ADD_SIGNAL(MethodInfo("plan_generated",
			PropertyInfo(Variant::STRING, "source_path"),
			PropertyInfo(Variant::DICTIONARY, "plan")));
}

AIMeshPostProcessor *AIMeshPostProcessor::get_singleton() {
	return singleton;
}

bool AIMeshPostProcessor::has_import_orchestrator() const {
	return _get_orchestrator() != nullptr;
}

bool AIMeshPostProcessor::is_ready() const {
	return has_import_orchestrator();
}

Dictionary AIMeshPostProcessor::describe_processing_scope(const String &p_source_path, const String &p_importer_name, const Dictionary &p_options) const {
	Dictionary scope;
	bool mesh_pass_enabled = false;
	Variant target_profile = String("default");
	Variant generate_lod_suggestions = true;
	Variant generate_topology_review = true;
	Variant generate_classification = true;

	if (has_import_orchestrator()) {
		mesh_pass_enabled = _get_orchestrator()->is_mesh_postprocess_enabled();
	}
	if (p_options.has("target_profile")) {
		target_profile = p_options["target_profile"];
	}
	if (p_options.has("generate_lod_suggestions")) {
		generate_lod_suggestions = p_options["generate_lod_suggestions"];
	}
	if (p_options.has("generate_topology_review")) {
		generate_topology_review = p_options["generate_topology_review"];
	}
	if (p_options.has("generate_classification")) {
		generate_classification = p_options["generate_classification"];
	}

	scope["source_path"] = p_source_path;
	scope["importer_name"] = p_importer_name;
	scope["mesh_pass_enabled"] = mesh_pass_enabled;
	scope["target_profile"] = target_profile;
	scope["generate_lod_suggestions"] = generate_lod_suggestions;
	scope["generate_topology_review"] = generate_topology_review;
	scope["generate_classification"] = generate_classification;
	scope["options"] = p_options;
	return scope;
}

Dictionary AIMeshPostProcessor::build_processing_plan(const String &p_source_path, const String &p_importer_name, const Dictionary &p_options) {
	planned_jobs++;

	Dictionary plan;
	const Dictionary scope = describe_processing_scope(p_source_path, p_importer_name, p_options);
	Array recommended_steps;
	Array deferred_steps;
	Array warnings;

	if (bool(scope["generate_topology_review"])) {
		recommended_steps.push_back("topology_review");
	}
	if (bool(scope["generate_classification"])) {
		recommended_steps.push_back("mesh_classification");
	}
	if (bool(scope["generate_lod_suggestions"])) {
		recommended_steps.push_back("lod_suggestion");
	}

	deferred_steps.push_back("mesh_rewrite");
	deferred_steps.push_back("auto_decimation");
	deferred_steps.push_back("collision_regeneration");
	warnings.push_back("AIMeshPostProcessor is currently a planning skeleton and does not mutate imported meshes.");

	plan["schema"] = "woodot_ai.mesh_postprocess_plan.v1";
	plan["scope"] = scope;
	plan["recommended_steps"] = recommended_steps;
	plan["deferred_steps"] = deferred_steps;
	plan["warnings"] = warnings;
	plan["requires_manual_review"] = true;
	plan["can_apply_automatically"] = false;

	generated_plans++;
	emit_signal(SNAME("plan_generated"), p_source_path, plan);
	return plan;
}

Dictionary AIMeshPostProcessor::build_review_report(const Dictionary &p_plan) const {
	Dictionary report;
	report["kind"] = "mesh_postprocess_review";
	report["ok"] = p_plan.has("schema");
	report["summary"] = "Mesh post-processing plan is advisory only in the current MVP.";
	report["plan"] = p_plan;
	report["requires_manual_review"] = true;
	return report;
}

Dictionary AIMeshPostProcessor::get_processor_status() const {
	Dictionary status;
	status["ready"] = is_ready();
	status["has_import_orchestrator"] = has_import_orchestrator();
	status["planned_jobs"] = static_cast<int64_t>(planned_jobs);
	status["generated_plans"] = static_cast<int64_t>(generated_plans);
	return status;
}

AIImportOrchestrator *AIMeshPostProcessor::_get_orchestrator() const {
	return AIImportOrchestrator::get_singleton();
}

AIMeshPostProcessor::AIMeshPostProcessor() {
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;
}

AIMeshPostProcessor::~AIMeshPostProcessor() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
