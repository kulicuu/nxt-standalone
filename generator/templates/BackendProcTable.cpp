//* Copyright (c) 2017 The Chromium Authors. All rights reserved.
//* Use of this source code is governed by a BSD-style license that can be
//* found in the LICENSE file.

#include "nxt/nxt.h"
#include "nxt/nxtcpp.h"

#include "{{namespace}}/GeneratedCodeIncludes.h"

#include <iostream>

namespace backend {
namespace {{namespace}} {

    namespace {

        //* Helper functions to check the value of enums
        {% for type in by_category["enum"] %}
            {% set cType = as_cType(type.name) %}
            bool CheckEnum{{cType}}({{cType}} value) {
                switch (value) {
                    {% for value in type.values %}
                        case {{as_cEnum(type.name, value.name)}}:
                            return true;
                    {% endfor %}
                    default:
                        return false;
                }
            }
        {% endfor %}

        {% for type in by_category["bitmask"] %}
            {% set cType = as_cType(type.name) %}
            bool CheckBitmask{{cType}}({{cType}} value) {
                return (value & ~{{type.full_mask}}) == 0;
            }
        {% endfor %}

        {% set methodsWithExtraValidation = (
            "CommandBufferBuilderGetResult",
        ) %}

        {% for type in by_category["object"] %}
            {% for method in native_methods(type) %}
                {% set suffix = as_MethodSuffix(type.name, method.name) %}

                //* Entry point without validation, forwards the arguments to the method directly
                {{as_backendType(method.return_type)}} NonValidating{{suffix}}(
                    {{-as_backendType(type)}} self
                    {%- for arg in method.arguments -%}
                        , {{as_annotated_backendType(arg)}}
                    {%- endfor -%}
                ) {
                    {% if method.return_type.name.canonical_case() != "void" %}
                        auto result =
                    {%- endif %}
                    self->{{method.name.CamelCase()}}(
                        {%- for arg in method.arguments -%}
                            {%- if not loop.first %}, {% endif -%}
                            {%- if arg.type.category in ["enum", "bitmask"] -%}
                                static_cast<nxt::{{as_cppType(arg.type.name)}}>({{as_varName(arg.name)}})
                            {%- else -%}
                                {{as_varName(arg.name)}}
                            {%- endif -%}
                        {%- endfor -%}
                    );
                    {% if method.return_type.name.canonical_case() != "void" %}
                        return reinterpret_cast<{{as_backendType(method.return_type)}}>(result);
                    {% endif %}
                }

                //* Autogenerated part of the entry point validation
                //*  - Check that enum and bitmaks are in the correct range
                //*  - Check that builders have not been consumed already
                //*  - Others TODO
                bool ValidateBase{{suffix}}(
                    {{-as_backendType(type)}} self
                    {%- for arg in method.arguments -%}
                        , {{as_annotated_backendType(arg)}}
                    {%- endfor -%}
                ) {
                    {% if type.is_builder and method.name.canonical_case() not in ("release", "reference") %}
                        if (self->WasConsumed()) return false;
                    {% else %}
                        (void) self;
                    {% endif %}
                    {% for arg in method.arguments %}
                        {% if arg.type.category == "enum" %}
                            if (!CheckEnum{{as_cType(arg.type.name)}}({{as_varName(arg.name)}})) return false;
                        {% elif arg.type.category == "bitmask" %}
                            if (!CheckBitmask{{as_cType(arg.type.name)}}({{as_varName(arg.name)}})) return false;
                        {% else %}
                            (void) {{as_varName(arg.name)}};
                        {% endif %}
                    {% endfor %}
                    return true;
                }

                //* Entry point with validation
                {{as_backendType(method.return_type)}} Validating{{suffix}}(
                    {{-as_backendType(type)}} self
                    {%- for arg in method.arguments -%}
                        , {{as_annotated_backendType(arg)}}
                    {%- endfor -%}
                ) {
                    //* Do the autogenerated checks
                    bool valid = ValidateBase{{suffix}}(self
                        {%- for arg in method.arguments -%}
                            , {{as_varName(arg.name)}}
                        {%- endfor -%}
                    );

                    {% if suffix in methodsWithExtraValidation %}
                        if (valid) {
                            valid = self->Validate{{method.name.CamelCase()}}(
                                {%- for arg in method.arguments -%}
                                    {% if not loop.first %}, {% endif %}{{as_varName(arg.name)}}
                                {%- endfor -%}
                            );
                        }
                    {% endif %}
                    //* TODO Do the hand-written checks if necessary
                    //* On success, forward the arguments to the method, else error out without calling it
                    if (!valid) {
                        // TODO get the device and give it the error?
                        std::cout << "Error in {{suffix}}" << std::endl;
                    }
                    {% if method.return_type.name.canonical_case() == "void" %}
                        if (!valid) return;
                    {% else %}
                        if (!valid) {
                            return {};
                        }
                        auto result =
                    {%- endif %}
                    self->{{method.name.CamelCase()}}(
                        {%- for arg in method.arguments -%}
                            {%- if not loop.first %}, {% endif -%}
                            {%- if arg.type.category in ["enum", "bitmask"] -%}
                                static_cast<nxt::{{as_cppType(arg.type.name)}}>({{as_varName(arg.name)}})
                            {%- else -%}
                                {{as_varName(arg.name)}}
                            {%- endif -%}
                        {%- endfor -%}
                    );
                    {% if method.return_type.name.canonical_case() != "void" %}
                        return reinterpret_cast<{{as_backendType(method.return_type)}}>(result);
                    {% endif %}
                }
            {% endfor %}
        {% endfor %}
    }

    nxtProcTable GetNonValidatingProcs() {
        nxtProcTable table;
        {% for type in by_category["object"] %}
            {% for method in native_methods(type) %}
                table.{{as_varName(type.name, method.name)}} = reinterpret_cast<{{as_cProc(type.name, method.name)}}>(NonValidating{{as_MethodSuffix(type.name, method.name)}});
            {% endfor %}
        {% endfor %}
        return table;
    }

    nxtProcTable GetValidatingProcs() {
        nxtProcTable table;
        {% for type in by_category["object"] %}
            {% for method in native_methods(type) %}
                table.{{as_varName(type.name, method.name)}} = reinterpret_cast<{{as_cProc(type.name, method.name)}}>(Validating{{as_MethodSuffix(type.name, method.name)}});
            {% endfor %}
        {% endfor %}
        return table;
    }
}
}