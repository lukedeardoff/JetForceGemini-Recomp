include_guard(GLOBAL)

include(CMakeParseArguments)

file(REAL_PATH "${CMAKE_CURRENT_LIST_DIR}/.." _JFG_GENERATED_REPOSITORY_ROOT)

function(_jfg_json_required_value output_name json member)
    string(JSON value ERROR_VARIABLE json_error GET "${json}" "${member}")
    if(NOT json_error STREQUAL "NOTFOUND")
        message(FATAL_ERROR "Invalid generated source manifest member '${member}': ${json_error}")
    endif()
    set(${output_name} "${value}" PARENT_SCOPE)
endfunction()

function(_jfg_json_require_type json member expected_type)
    string(JSON actual_type ERROR_VARIABLE json_error TYPE "${json}" "${member}")
    if(NOT json_error STREQUAL "NOTFOUND")
        message(FATAL_ERROR "Invalid generated source manifest member '${member}': ${json_error}")
    endif()
    if(NOT actual_type STREQUAL expected_type)
        message(FATAL_ERROR
            "Generated source manifest member '${member}' must have type ${expected_type}"
        )
    endif()
endfunction()

function(_jfg_json_required_array output_name json member)
    string(JSON array_length ERROR_VARIABLE json_error LENGTH "${json}" "${member}")
    if(NOT json_error STREQUAL "NOTFOUND")
        message(FATAL_ERROR "Invalid generated source manifest array '${member}': ${json_error}")
    endif()

    set(values "")
    if(array_length GREATER 0)
        math(EXPR last_index "${array_length} - 1")
        foreach(index RANGE 0 ${last_index})
            string(JSON value_type ERROR_VARIABLE json_error TYPE "${json}" "${member}" ${index})
            if(NOT json_error STREQUAL "NOTFOUND" OR NOT value_type STREQUAL "STRING")
                message(FATAL_ERROR "Generated source manifest '${member}' entries must be strings")
            endif()
            string(JSON value ERROR_VARIABLE json_error GET "${json}" "${member}" ${index})
            if(NOT json_error STREQUAL "NOTFOUND")
                message(FATAL_ERROR "Invalid '${member}' entry ${index}: ${json_error}")
            endif()
            if(value MATCHES "[;\r\n]" OR value MATCHES "\\$<")
                message(FATAL_ERROR
                    "Generated source manifest '${member}' entries contain forbidden CMake syntax"
                )
            endif()
            list(APPEND values "${value}")
        endforeach()
    endif()
    set(${output_name} "${values}" PARENT_SCOPE)
endfunction()

function(_jfg_validate_relative_path relative_path description)
    if(relative_path STREQUAL "" OR IS_ABSOLUTE "${relative_path}")
        message(FATAL_ERROR "${description} must be a non-empty relative path")
    endif()
    if(relative_path MATCHES "[;\r\n]" OR relative_path MATCHES "\\$<")
        message(FATAL_ERROR "${description} contains forbidden CMake syntax")
    endif()
    if(relative_path MATCHES "\\\\" OR relative_path MATCHES "(^|/)\\.\\.?(/|$)")
        message(FATAL_ERROR "${description} must use a canonical contained path")
    endif()
endfunction()

function(_jfg_resolve_contained_path output_name root relative_path description)
    _jfg_validate_relative_path("${relative_path}" "${description}")
    set(candidate "${root}/${relative_path}")
    if(NOT EXISTS "${candidate}")
        message(FATAL_ERROR "${description} does not exist")
    endif()

    file(REAL_PATH "${candidate}" real_candidate)
    file(RELATIVE_PATH contained_relative "${root}" "${real_candidate}")
    if(contained_relative MATCHES "^\\.\\.(/|$)" OR IS_ABSOLUTE "${contained_relative}")
        message(FATAL_ERROR "${description} escapes its generated root")
    endif()

    set(${output_name} "${real_candidate}" PARENT_SCOPE)
endfunction()

function(_jfg_validate_generated_list list_name root allow_empty extension_pattern)
    set(paths ${${list_name}})
    if(NOT paths AND NOT allow_empty)
        message(FATAL_ERROR "${list_name} must contain at least one source")
    endif()

    set(sorted_paths ${paths})
    list(SORT sorted_paths)
    if(NOT "${paths}" STREQUAL "${sorted_paths}")
        message(FATAL_ERROR "${list_name} must be sorted for deterministic builds")
    endif()

    set(unique_paths ${paths})
    list(REMOVE_DUPLICATES unique_paths)
    list(LENGTH paths path_count)
    list(LENGTH unique_paths unique_path_count)
    if(NOT path_count EQUAL unique_path_count)
        message(FATAL_ERROR "${list_name} contains duplicate paths")
    endif()

    set(resolved_paths "")
    foreach(relative_path IN LISTS paths)
        if(NOT relative_path MATCHES "${extension_pattern}")
            message(FATAL_ERROR "${list_name} contains an unsupported source extension")
        endif()
        _jfg_resolve_contained_path(
            absolute_path
            "${root}"
            "${relative_path}"
            "${list_name} entry"
        )
        if(IS_DIRECTORY "${absolute_path}")
            message(FATAL_ERROR "${list_name} entries must be files")
        endif()
        list(APPEND resolved_paths "${absolute_path}")
    endforeach()

    set(${list_name}_RESOLVED "${resolved_paths}" PARENT_SCOPE)
endfunction()

function(_jfg_require_private_root root)
    set(source_root "${_JFG_GENERATED_REPOSITORY_ROOT}")
    file(RELATIVE_PATH root_relative "${source_root}" "${root}")

    # An explicitly supplied root outside the worktree cannot be committed to
    # this repository. Roots inside it must be covered by a git ignore rule.
    if(root_relative MATCHES "^\\.\\.(/|$)" OR IS_ABSOLUTE "${root_relative}")
        return()
    endif()

    find_package(Git QUIET)
    if(NOT GIT_FOUND)
        message(FATAL_ERROR "Git is required to verify an in-worktree private root")
    endif()

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${source_root}" check-ignore --quiet -- "${root_relative}"
        RESULT_VARIABLE ignore_result
        ERROR_QUIET
    )
    if(NOT ignore_result EQUAL 0)
        message(FATAL_ERROR "An in-worktree private root must be ignored by git")
    endif()
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${source_root}" ls-files -- "${root_relative}"
        RESULT_VARIABLE tracked_query_result
        OUTPUT_VARIABLE tracked_paths
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT tracked_query_result EQUAL 0 OR NOT tracked_paths STREQUAL "")
        message(FATAL_ERROR "A private root must not contain tracked inputs")
    endif()
endfunction()

function(jfg_resolve_private_root output_name requested_root label)
    if(NOT output_name MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
        message(FATAL_ERROR "Invalid private-root output variable")
    endif()
    if(requested_root STREQUAL "" OR
       requested_root MATCHES "[;\r\n]" OR
       requested_root MATCHES "\\$<")
        message(FATAL_ERROR "${label} must name one explicit private directory")
    endif()
    if(NOT IS_DIRECTORY "${requested_root}")
        message(FATAL_ERROR "${label} does not exist or is not a directory")
    endif()
    file(REAL_PATH "${requested_root}" resolved_root
        BASE_DIRECTORY "${_JFG_GENERATED_REPOSITORY_ROOT}")
    _jfg_require_private_root("${resolved_root}")
    set(${output_name} "${resolved_root}" PARENT_SCOPE)
endfunction()

# Resolve one explicit private file.  In-worktree inputs must be both ignored
# and untracked; external inputs are allowed because they cannot be committed
# here.  Reject syntax which could alter CMake list or generator evaluation.
function(jfg_resolve_private_file output_name requested_file label)
    if(NOT output_name MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
        message(FATAL_ERROR "Invalid private-file output variable")
    endif()
    if(requested_file STREQUAL "" OR requested_file MATCHES "[;\r\n]" OR
       requested_file MATCHES "\\$<")
        message(FATAL_ERROR "${label} must name one explicit private file")
    endif()
    if(IS_DIRECTORY "${requested_file}" OR NOT EXISTS "${requested_file}" OR
       IS_SYMLINK "${requested_file}")
        message(FATAL_ERROR "${label} does not exist or is not a regular file")
    endif()
    file(REAL_PATH "${requested_file}" resolved_file
        BASE_DIRECTORY "${_JFG_GENERATED_REPOSITORY_ROOT}")
    if(IS_DIRECTORY "${resolved_file}")
        message(FATAL_ERROR "${label} does not exist or is not a regular file")
    endif()
    file(RELATIVE_PATH file_relative "${_JFG_GENERATED_REPOSITORY_ROOT}" "${resolved_file}")
    if(NOT file_relative MATCHES "^\\.\\.(/|$)" AND NOT IS_ABSOLUTE "${file_relative}")
        find_package(Git QUIET)
        if(NOT GIT_FOUND)
            message(FATAL_ERROR "Git is required to verify an in-worktree private file")
        endif()
        execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${_JFG_GENERATED_REPOSITORY_ROOT}"
            check-ignore --quiet -- "${file_relative}" RESULT_VARIABLE ignore_result ERROR_QUIET)
        execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${_JFG_GENERATED_REPOSITORY_ROOT}"
            ls-files -- "${file_relative}" RESULT_VARIABLE tracked_result
            OUTPUT_VARIABLE tracked_paths ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(NOT ignore_result EQUAL 0 OR NOT tracked_result EQUAL 0 OR NOT tracked_paths STREQUAL "")
            message(FATAL_ERROR "An in-worktree private file must be ignored by git and untracked")
        endif()
    endif()
    set(${output_name} "${resolved_file}" PARENT_SCOPE)
endfunction()

function(_jfg_validate_manifest_members json)
    set(allowed_members
        "baseline_body_sources"
        "alternate_entry_thunk_sources"
        "game_patch_function_count"
        "link_smoke_sources"
        "n64recomp_include"
        "normal_wrapper_sources"
        "patch_sources"
        "support_sources"
        "table_support_sources"
        "symbol_inventory"
        "symbol_inventory_sha256"
        "version"
    )

    string(JSON member_count ERROR_VARIABLE json_error LENGTH "${json}")
    if(NOT json_error STREQUAL "NOTFOUND")
        message(FATAL_ERROR "Invalid generated source manifest: ${json_error}")
    endif()
    if(member_count GREATER 0)
        math(EXPR last_index "${member_count} - 1")
        foreach(index RANGE 0 ${last_index})
            string(JSON member_name ERROR_VARIABLE json_error MEMBER "${json}" ${index})
            if(NOT json_error STREQUAL "NOTFOUND")
                message(FATAL_ERROR "Invalid generated source manifest: ${json_error}")
            endif()
            list(FIND allowed_members "${member_name}" allowed_index)
            if(allowed_index EQUAL -1)
                message(FATAL_ERROR "Unknown generated source manifest member: ${member_name}")
            endif()
        endforeach()
    endif()
endfunction()

function(_jfg_set_generated_target target recomp_include generated_root)
    target_include_directories(${target} SYSTEM PRIVATE
        "${recomp_include}"
        "${generated_root}"
    )
    target_compile_features(${target} PRIVATE c_std_11)
    set(JFG_MSVC_GENERATED_COMPILE_JOBS "1" CACHE STRING
        "MSVC compiler processes per generated-code target (1 preserves default)")
    if(NOT JFG_MSVC_GENERATED_COMPILE_JOBS MATCHES "^([1-9]|[12][0-9]|3[0-2])$")
        message(FATAL_ERROR "JFG_MSVC_GENERATED_COMPILE_JOBS must be 1..32")
    endif()
    if(MSVC AND CMAKE_GENERATOR MATCHES "Visual Studio" AND
       JFG_MSVC_GENERATED_COMPILE_JOBS GREATER 1)
        target_compile_options(${target} PRIVATE "/MP${JFG_MSVC_GENERATED_COMPILE_JOBS}")
    endif()
    # The executor unwinds ExecutorShutdown through generated guest frames.
    # GCC/Clang emit no unwind tables for C by default, which turns that into
    # std::terminate; MSVC x64 always emits table-based unwind information.
    if(NOT MSVC AND NOT CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_compile_options(${target} PRIVATE
            $<$<COMPILE_LANGUAGE:C>:-fexceptions>
        )
    endif()
    jfg_set_generated_warnings(${target})
endfunction()

function(_jfg_generate_object_list output target)
    file(GENERATE
        OUTPUT "${output}"
        CONTENT "$<JOIN:$<TARGET_OBJECTS:${target}>,\n>\n"
    )
endfunction()

function(_jfg_add_object_shape_audit name inventory_copy)
    cmake_parse_arguments(
        ARG
        ""
        "BODY_TARGET;WRAPPER_TARGET;PATCH_TARGET;SUPPORT_TARGET;RUNTIME_TARGET;SMOKE_TARGET"
        ""
        ${ARGN}
    )
    # ASan deliberately adds compiler/runtime metadata symbols to every object,
    # so the release object-shape inventory cannot apply to an instrumented
    # build. The ordinary Phase 6 binary still depends on the strict audit;
    # this target preserves dependency topology only for the separately
    # identified sanitizer execution.
    if(JFG_ENABLE_ADDRESS_SANITIZER)
        add_custom_target(${name}_object_shape_audit)
        return()
    endif()
    foreach(required BODY_TARGET WRAPPER_TARGET SUPPORT_TARGET RUNTIME_TARGET SMOKE_TARGET)
        if(NOT ARG_${required} OR NOT TARGET ${ARG_${required}})
            message(FATAL_ERROR "Generated object audit requires ${required}")
        endif()
    endforeach()

    find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)
    if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        get_filename_component(msvc_compiler_directory "${CMAKE_C_COMPILER}" DIRECTORY)
        find_program(symbol_tool NAMES dumpbin HINTS "${msvc_compiler_directory}" REQUIRED)
        set(symbol_tool_kind dumpbin)
    else()
        if(NOT CMAKE_NM)
            message(FATAL_ERROR "A GNU/LLVM nm-compatible symbol tool is required")
        endif()
        set(symbol_tool "${CMAKE_NM}")
        set(symbol_tool_kind nm)
    endif()

    set(audit_directory "${CMAKE_CURRENT_BINARY_DIR}/generated-object-audit/${name}")
    file(MAKE_DIRECTORY "${audit_directory}")
    set(body_list "${audit_directory}/baseline-body-$<CONFIG>.txt")
    set(wrapper_list "${audit_directory}/normal-wrapper-$<CONFIG>.txt")
    set(patch_list "${audit_directory}/patch-$<CONFIG>.txt")
    set(support_list "${audit_directory}/support-$<CONFIG>.txt")
    set(runtime_list "${audit_directory}/runtime-$<CONFIG>.txt")
    set(smoke_list "${audit_directory}/smoke-$<CONFIG>.txt")
    set(host_function_inventory "${audit_directory}/host-function-$<CONFIG>.json")
    set(host_data_inventory "${audit_directory}/host-data-$<CONFIG>.json")
    set(object_role_inventory "${audit_directory}/object-role-$<CONFIG>.json")
    _jfg_generate_object_list("${body_list}" ${ARG_BODY_TARGET})
    _jfg_generate_object_list("${wrapper_list}" ${ARG_WRAPPER_TARGET})
    if(ARG_PATCH_TARGET)
        if(NOT TARGET ${ARG_PATCH_TARGET})
            message(FATAL_ERROR "Generated object audit PATCH_TARGET does not exist")
        endif()
        _jfg_generate_object_list("${patch_list}" ${ARG_PATCH_TARGET})
    else()
        file(GENERATE OUTPUT "${patch_list}" CONTENT "")
    endif()
    _jfg_generate_object_list("${support_list}" ${ARG_SUPPORT_TARGET})
    _jfg_generate_object_list("${runtime_list}" ${ARG_RUNTIME_TARGET})
    _jfg_generate_object_list("${smoke_list}" ${ARG_SMOKE_TARGET})

    set(audit_dependencies
        ${ARG_BODY_TARGET}
        ${ARG_WRAPPER_TARGET}
        ${ARG_SUPPORT_TARGET}
        ${ARG_RUNTIME_TARGET}
        ${ARG_SMOKE_TARGET}
        "${_JFG_GENERATED_REPOSITORY_ROOT}/scripts/audit_generated_objects.py"
        "${inventory_copy}"
    )
    if(ARG_PATCH_TARGET)
        list(APPEND audit_dependencies ${ARG_PATCH_TARGET})
    endif()

    add_custom_target(${name}_object_shape_audit ALL
        COMMAND "${Python3_EXECUTABLE}"
            "${_JFG_GENERATED_REPOSITORY_ROOT}/scripts/audit_generated_objects.py"
            --tool-kind "${symbol_tool_kind}"
            --tool "${symbol_tool}"
            --compiler-frontend-variant "${CMAKE_C_COMPILER_FRONTEND_VARIANT}"
            --inventory "${inventory_copy}"
            --baseline-body-objects "${body_list}"
            --normal-wrapper-objects "${wrapper_list}"
            --patch-objects "${patch_list}"
            --support-objects "${support_list}"
            --runtime-objects "${runtime_list}"
            --link-smoke-objects "${smoke_list}"
            --host-function-inventory-output "${host_function_inventory}"
            --host-data-inventory-output "${host_data_inventory}"
            --object-role-inventory-output "${object_role_inventory}"
        DEPENDS ${audit_dependencies}
        VERBATIM
    )
endfunction()

# The generated root contains only data manifests and generated C/C++ inputs.
# It never supplies CMake code, the patch archive anchor, or the minimal runtime.
function(jfg_add_generated_code)
    cmake_parse_arguments(
        ARG
        "REQUIRE_PRIVATE_ROOT;ALLOW_TRACKED_ROM_FREE_FIXTURE"
        "NAME;ROOT"
        ""
        ${ARGN}
    )

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown jfg_add_generated_code arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT ARG_NAME)
        message(FATAL_ERROR "jfg_add_generated_code requires NAME")
    endif()
    if(NOT ARG_ROOT)
        message(FATAL_ERROR "jfg_add_generated_code requires an explicit ROOT")
    endif()
    if(ARG_ROOT MATCHES "[;\r\n]" OR ARG_ROOT MATCHES "\\$<")
        message(FATAL_ERROR "JFG_GENERATED_ROOT contains forbidden CMake syntax")
    endif()
    if(ARG_REQUIRE_PRIVATE_ROOT AND ARG_ALLOW_TRACKED_ROM_FREE_FIXTURE)
        message(FATAL_ERROR "A generated root cannot select both root policies")
    endif()
    if(NOT ARG_REQUIRE_PRIVATE_ROOT AND NOT ARG_ALLOW_TRACKED_ROM_FREE_FIXTURE)
        message(FATAL_ERROR "The generated root policy must be selected explicitly")
    endif()

    if(NOT IS_DIRECTORY "${ARG_ROOT}")
        message(FATAL_ERROR "The explicitly supplied generated root does not exist")
    endif()
    file(REAL_PATH "${ARG_ROOT}" generated_root BASE_DIRECTORY "${_JFG_GENERATED_REPOSITORY_ROOT}")
    if(ARG_REQUIRE_PRIVATE_ROOT)
        _jfg_require_private_root("${generated_root}")
    else()
        file(REAL_PATH
            "${_JFG_GENERATED_REPOSITORY_ROOT}/tests/fixtures/generated-code-synthetic"
            allowed_fixture_root
        )
        if(NOT generated_root STREQUAL allowed_fixture_root)
            message(FATAL_ERROR
                "ALLOW_TRACKED_ROM_FREE_FIXTURE is restricted to the repository's synthetic fixture"
            )
        endif()
    endif()

    _jfg_resolve_contained_path(
        source_manifest
        "${generated_root}"
        "sources.json"
        "Generated source manifest"
    )
    if(IS_DIRECTORY "${source_manifest}")
        message(FATAL_ERROR "Generated source manifest must be a file")
    endif()
    find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)
    set(preparation_script
        "${_JFG_GENERATED_REPOSITORY_ROOT}/scripts/prepare_generated_sources.py"
    )
    set(preparation_directory
        "${CMAKE_CURRENT_BINARY_DIR}/generated-source-preparation/${ARG_NAME}"
    )
    set(preparation_command
        "${Python3_EXECUTABLE}" "${preparation_script}"
        --root "${generated_root}"
        --output "${preparation_directory}"
    )
    if(ARG_REQUIRE_PRIVATE_ROOT)
        # Private roots must be regenerated when the public normalizer changes.
        # This revision check is not an authenticity or attestation mechanism.
        list(APPEND preparation_command --require-normalizer-revision)
    endif()
    execute_process(
        COMMAND ${preparation_command}
        RESULT_VARIABLE preparation_result
        OUTPUT_QUIET
        ERROR_VARIABLE preparation_error
    )
    if(NOT preparation_result EQUAL 0)
        string(STRIP "${preparation_error}" preparation_error)
        message(FATAL_ERROR "${preparation_error}")
    endif()

    file(STRINGS "${preparation_directory}/baseline-body.txt" JFG_BODY_INPUTS_RESOLVED)
    file(STRINGS "${preparation_directory}/normal-wrapper.txt" JFG_WRAPPER_INPUTS_RESOLVED)
    file(STRINGS "${preparation_directory}/patch.txt" JFG_PATCH_INPUTS_RESOLVED)
    file(STRINGS "${preparation_directory}/support.txt" JFG_SUPPORT_INPUTS_RESOLVED)
    file(STRINGS "${preparation_directory}/link-smoke.txt" JFG_LINK_SMOKE_INPUTS_RESOLVED)
    file(STRINGS "${preparation_directory}/recomp-include.txt" recomp_include LIMIT_COUNT 1)
    file(STRINGS "${preparation_directory}/inventory-source.txt" symbol_inventory LIMIT_COUNT 1)
    set(cpu_section_inventory "")
    if(EXISTS "${preparation_directory}/cpu-inventory-source.txt")
        file(STRINGS
            "${preparation_directory}/cpu-inventory-source.txt"
            cpu_section_inventory
            LIMIT_COUNT 1
        )
    endif()
    set(inventory_copy "${preparation_directory}/symbol-inventory.json")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${source_manifest}"
        "${symbol_inventory}"
        "${preparation_script}"
    )
    if(cpu_section_inventory)
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
            "${cpu_section_inventory}"
        )
    endif()
    if(ARG_REQUIRE_PRIVATE_ROOT)
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
            "${_JFG_GENERATED_REPOSITORY_ROOT}/scripts/build_private_generated_root.py"
        )
    endif()

    add_library(${ARG_NAME}_baseline_body_objects OBJECT ${JFG_BODY_INPUTS_RESOLVED})
    _jfg_set_generated_target(${ARG_NAME}_baseline_body_objects "${recomp_include}" "${generated_root}")

    add_library(${ARG_NAME}_normal_wrapper_objects OBJECT ${JFG_WRAPPER_INPUTS_RESOLVED})
    _jfg_set_generated_target(${ARG_NAME}_normal_wrapper_objects "${recomp_include}" "${generated_root}")

    add_library(${ARG_NAME}_support_objects OBJECT ${JFG_SUPPORT_INPUTS_RESOLVED})
    _jfg_set_generated_target(${ARG_NAME}_support_objects "${recomp_include}" "${generated_root}")

    add_library(${ARG_NAME}_normal_wrappers STATIC)
    target_link_libraries(${ARG_NAME}_normal_wrappers PRIVATE
        ${ARG_NAME}_normal_wrapper_objects
    )
    add_library(${ARG_NAME}_baseline STATIC)
    target_link_libraries(${ARG_NAME}_baseline PRIVATE
        ${ARG_NAME}_baseline_body_objects
        ${ARG_NAME}_normal_wrapper_objects
        ${ARG_NAME}_support_objects
    )

    set(patch_anchor_source "${_JFG_GENERATED_REPOSITORY_ROOT}/src/runtime/recomp_support/patch_archive_anchor.c")
    add_library(${ARG_NAME}_patch_anchor_objects OBJECT "${patch_anchor_source}")
    jfg_set_project_warnings(${ARG_NAME}_patch_anchor_objects)
    target_compile_features(${ARG_NAME}_patch_anchor_objects PRIVATE c_std_11)

    set(patch_target_argument "")
    if(JFG_PATCH_INPUTS_RESOLVED)
        add_library(${ARG_NAME}_patch_objects OBJECT ${JFG_PATCH_INPUTS_RESOLVED})
        _jfg_set_generated_target(${ARG_NAME}_patch_objects "${recomp_include}" "${generated_root}")
        set(patch_target_argument PATCH_TARGET ${ARG_NAME}_patch_objects)
    endif()
    add_library(${ARG_NAME}_patch STATIC)
    target_link_libraries(${ARG_NAME}_patch PRIVATE ${ARG_NAME}_patch_anchor_objects)
    if(TARGET ${ARG_NAME}_patch_objects)
        target_link_libraries(${ARG_NAME}_patch PRIVATE ${ARG_NAME}_patch_objects)
    endif()

    set(minimal_runtime_source "${_JFG_GENERATED_REPOSITORY_ROOT}/src/runtime/recomp_support/minimal_runtime.cpp")
    add_library(${ARG_NAME}_minimal_runtime_objects OBJECT "${minimal_runtime_source}")
    target_include_directories(${ARG_NAME}_minimal_runtime_objects PRIVATE
        "${_JFG_GENERATED_REPOSITORY_ROOT}/include")
    target_include_directories(${ARG_NAME}_minimal_runtime_objects SYSTEM PRIVATE
        "${recomp_include}"
        "${generated_root}"
    )
    jfg_set_project_warnings(${ARG_NAME}_minimal_runtime_objects)
    target_compile_features(${ARG_NAME}_minimal_runtime_objects PRIVATE cxx_std_20)
    add_library(${ARG_NAME}_minimal_runtime STATIC)
    target_link_libraries(${ARG_NAME}_minimal_runtime PRIVATE
        ${ARG_NAME}_minimal_runtime_objects
    )

    # Private Linux trap probes must replace only the generated ABI's fatal
    # bridge exports while retaining the same checked dispatch/runtime state.
    # Keep this out of the normal link graph so ordinary builds cannot acquire
    # the evidence-only process boundary accidentally.
    add_library(${ARG_NAME}_g2_trap_runtime_objects OBJECT "${minimal_runtime_source}")
    target_include_directories(${ARG_NAME}_g2_trap_runtime_objects PRIVATE
        "${_JFG_GENERATED_REPOSITORY_ROOT}/include")
    target_include_directories(${ARG_NAME}_g2_trap_runtime_objects SYSTEM PRIVATE
        "${recomp_include}"
        "${generated_root}"
    )
    target_compile_definitions(${ARG_NAME}_g2_trap_runtime_objects PRIVATE
        JFG_G2_TRAP_PROBE_BRIDGES=1
    )
    jfg_set_project_warnings(${ARG_NAME}_g2_trap_runtime_objects)
    target_compile_features(${ARG_NAME}_g2_trap_runtime_objects PRIVATE cxx_std_20)
    add_library(${ARG_NAME}_g2_trap_runtime STATIC)
    target_link_libraries(${ARG_NAME}_g2_trap_runtime PRIVATE
        ${ARG_NAME}_g2_trap_runtime_objects
    )

    add_library(${ARG_NAME}_link_smoke_objects OBJECT ${JFG_LINK_SMOKE_INPUTS_RESOLVED})
    target_include_directories(${ARG_NAME}_link_smoke_objects SYSTEM PRIVATE
        "${recomp_include}"
        "${generated_root}"
    )
    jfg_set_generated_warnings(${ARG_NAME}_link_smoke_objects)
    target_compile_features(${ARG_NAME}_link_smoke_objects PRIVATE c_std_11 cxx_std_20)

    _jfg_add_object_shape_audit(
        ${ARG_NAME}
        "${inventory_copy}"
        BODY_TARGET ${ARG_NAME}_baseline_body_objects
        WRAPPER_TARGET ${ARG_NAME}_normal_wrapper_objects
        ${patch_target_argument}
        SUPPORT_TARGET ${ARG_NAME}_support_objects
        RUNTIME_TARGET ${ARG_NAME}_minimal_runtime_objects
        SMOKE_TARGET ${ARG_NAME}_link_smoke_objects
    )
    add_dependencies(${ARG_NAME}_normal_wrappers ${ARG_NAME}_object_shape_audit)
    add_dependencies(${ARG_NAME}_baseline ${ARG_NAME}_object_shape_audit)
    add_dependencies(${ARG_NAME}_patch ${ARG_NAME}_object_shape_audit)
    add_dependencies(${ARG_NAME}_minimal_runtime ${ARG_NAME}_object_shape_audit)

    # Archive order is part of the patch contract. The generated smoke object
    # holds explicit references to the closed private inventory before these
    # archives are searched.
    add_library(${ARG_NAME}_link INTERFACE)
    target_link_libraries(${ARG_NAME}_link INTERFACE
        ${ARG_NAME}_patch
        ${ARG_NAME}_baseline
        ${ARG_NAME}_minimal_runtime
    )
    add_library(${ARG_NAME}_g2_trap_link INTERFACE)
    target_link_libraries(${ARG_NAME}_g2_trap_link INTERFACE
        ${ARG_NAME}_patch
        ${ARG_NAME}_baseline
        ${ARG_NAME}_g2_trap_runtime
    )
endfunction()

function(jfg_add_generated_code_tests)
    cmake_parse_arguments(
        ARG
        ""
        "NAME;LINK_SMOKE_SOURCE;BASELINE_AUDIT_SOURCE;PATCH_AUDIT_SOURCE"
        ""
        ${ARGN}
    )

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown jfg_add_generated_code_tests arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    foreach(required_argument NAME LINK_SMOKE_SOURCE BASELINE_AUDIT_SOURCE PATCH_AUDIT_SOURCE)
        if(NOT ARG_${required_argument})
            message(FATAL_ERROR "jfg_add_generated_code_tests requires ${required_argument}")
        endif()
    endforeach()
    foreach(required_target
        baseline_body_objects
        normal_wrapper_objects
        support_objects
        patch_anchor_objects
        normal_wrappers
        minimal_runtime_objects
        link_smoke_objects
        object_shape_audit
        link
    )
        if(NOT TARGET ${ARG_NAME}_${required_target})
            message(FATAL_ERROR "Missing generated target: ${ARG_NAME}_${required_target}")
        endif()
    endforeach()

    add_executable(${ARG_NAME}_link_smoke
        "${ARG_LINK_SMOKE_SOURCE}"
        $<TARGET_OBJECTS:${ARG_NAME}_link_smoke_objects>
    )
    target_link_libraries(${ARG_NAME}_link_smoke PRIVATE ${ARG_NAME}_link)
    add_dependencies(${ARG_NAME}_link_smoke ${ARG_NAME}_object_shape_audit)
    jfg_set_project_warnings(${ARG_NAME}_link_smoke)
    target_compile_features(${ARG_NAME}_link_smoke PRIVATE cxx_std_20)

    add_executable(${ARG_NAME}_baseline_audit
        "${ARG_BASELINE_AUDIT_SOURCE}"
    )
    target_link_libraries(${ARG_NAME}_baseline_audit PRIVATE
        ${ARG_NAME}_baseline_body_objects
        ${ARG_NAME}_normal_wrapper_objects
        ${ARG_NAME}_support_objects
        ${ARG_NAME}_minimal_runtime_objects
    )
    add_dependencies(${ARG_NAME}_baseline_audit ${ARG_NAME}_object_shape_audit)
    jfg_set_project_warnings(${ARG_NAME}_baseline_audit)
    target_compile_features(${ARG_NAME}_baseline_audit PRIVATE cxx_std_20)

    add_executable(${ARG_NAME}_patch_audit
        "${ARG_PATCH_AUDIT_SOURCE}"
    )
    target_link_libraries(${ARG_NAME}_patch_audit PRIVATE
        ${ARG_NAME}_patch_anchor_objects
        ${ARG_NAME}_baseline_body_objects
        ${ARG_NAME}_support_objects
        ${ARG_NAME}_minimal_runtime_objects
    )
    if(TARGET ${ARG_NAME}_patch_objects)
        target_link_libraries(${ARG_NAME}_patch_audit PRIVATE ${ARG_NAME}_patch_objects)
    endif()
    target_link_libraries(${ARG_NAME}_patch_audit PRIVATE ${ARG_NAME}_normal_wrappers)
    add_dependencies(${ARG_NAME}_patch_audit ${ARG_NAME}_object_shape_audit)
    jfg_set_project_warnings(${ARG_NAME}_patch_audit)
    target_compile_features(${ARG_NAME}_patch_audit PRIVATE cxx_std_20)

    # These audits retain guest bodies for linking without executing them.
    # Original-timing generated roots need fail-closed hook definitions too.
    foreach(audit_kind IN ITEMS link_smoke baseline_audit patch_audit)
        target_sources(${ARG_NAME}_${audit_kind} PRIVATE
            "${PROJECT_SOURCE_DIR}/tests/generated_timing_link_stubs.cpp")
    endforeach()

    add_test(NAME ${ARG_NAME}.link_smoke COMMAND ${ARG_NAME}_link_smoke)
    add_test(NAME ${ARG_NAME}.baseline_audit COMMAND ${ARG_NAME}_baseline_audit)
    add_test(NAME ${ARG_NAME}.patch_audit COMMAND ${ARG_NAME}_patch_audit)
endfunction()
