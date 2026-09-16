# Shared MSVC compiler options (spec section 38).
# Kept in its own module so CMakeLists.txt stays focused on target
# definitions rather than compiler-flag bookkeeping.

function(msm_apply_compiler_options target)
    if (MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            /utf-8
        )
        # /WX (warnings-as-errors) is opt-in via this CMake option rather
        # than always-on, so a normal developer build is not blocked by a
        # warning while iterating (spec section 38: "/WX ... trong CI hoặc
        # development build" - i.e. opt-in, not default).
        if (MSM_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    endif()

    target_compile_features(${target} PRIVATE cxx_std_20)
endfunction()
