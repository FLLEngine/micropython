/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 * Copyright (c) 2014-2017 Paul Sokolovsky
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "py/builtin.h"
#include "py/objlist.h"
#include "py/objtuple.h"
#include "py/objstr.h"
#include "py/objint.h"
#include "py/objtype.h"
#include "py/stream.h"
#include "py/smallint.h"
#include "py/runtime.h"
#include "genhdr/mpversion.h"

#if MICROPY_PY_SYS

// defined per port; type of these is irrelevant, just need pointer
extern struct _mp_dummy_t mp_sys_stdin_obj;
extern struct _mp_dummy_t mp_sys_stdout_obj;
extern struct _mp_dummy_t mp_sys_stderr_obj;

#if MICROPY_PY_IO && MICROPY_PY_SYS_STDFILES
const mp_print_t mp_sys_stdout_print = {&mp_sys_stdout_obj, mp_stream_write_adaptor};
#endif

// version - Python language version that this implementation conforms to, as a string
STATIC const MP_DEFINE_STR_OBJ(version_obj, "3.4.0");

// version_info - Python language version that this implementation conforms to, as a tuple of ints
#define I(n) MP_OBJ_NEW_SMALL_INT(n)
// TODO: CPython is now at 5-element array, but save 2 els so far...
STATIC const mp_obj_tuple_t mp_sys_version_info_obj = {{&mp_type_tuple}, 3, {I(3), I(4), I(0)}};

STATIC const MP_DEFINE_STR_OBJ(pybricks_micropython_obj, "pybricks-micropython");
STATIC const MP_DEFINE_STR_OBJ(pybricks_version_level_obj, "idgaf");
STATIC const MP_DEFINE_STR_OBJ(git_tag_obj, MICROPY_GIT_TAG);
STATIC const MP_DEFINE_STR_OBJ(build_date_obj, MICROPY_BUILD_DATE);

#if MICROPY_PY_ATTRTUPLE
STATIC const qstr pybricks_version_fields[] = {
    MP_QSTR_major, MP_QSTR_minor, MP_QSTR_micro, MP_QSTR_releaselevel, MP_QSTR_serial
};
STATIC MP_DEFINE_ATTRTUPLE(
    pybricks_version_obj,
    pybricks_version_fields,
    5,
        I(1),
        I(1),
        I(1),
        MP_ROM_PTR(&pybricks_version_level_obj),
        I(1)
);

STATIC const qstr impl_fields[] = {
    MP_QSTR_name, MP_QSTR_version, MP_QSTR_hexversion, MP_QSTR__git, MP_QSTR__date
};
STATIC MP_DEFINE_ATTRTUPLE(
    mp_sys_implementation_obj,
    impl_fields,
    5,
        MP_ROM_PTR(&pybricks_micropython_obj),
        MP_ROM_PTR(&pybricks_version_obj),
        I(1),
        MP_ROM_PTR(&git_tag_obj),
        MP_ROM_PTR(&build_date_obj)
);
#else
STATIC const mp_rom_obj_tuple_t pybricks_version_obj = {
    {&mp_type_tuple},
    5,
    {
        I(PYBRICKS_VERSION_MAJOR),
        I(PYBRICKS_VERSION_MINOR),
        I(PYBRICKS_VERSION_MICRO),
        I(PYBRICKS_VERSION_MICRO),
        MP_ROM_PTR(&pybricks_version_level_obj),
        I(PYBRICKS_VERSION_SERIAL),
    }
};
STATIC const mp_rom_obj_tuple_t mp_sys_implementation_obj = {
    {&mp_type_tuple},
    5,
    {
        MP_ROM_PTR(&pybricks_micropython_obj),
        MP_ROM_PTR(&pybricks_version_obj),
        I(PYBRICKS_HEXVERSION),
        MP_ROM_PTR(&git_tag_obj),
        MP_ROM_PTR(&build_date_obj),
    }
};
#endif

#undef I

#ifdef MICROPY_PY_SYS_PLATFORM
// platform - the platform that MicroPython is running on
STATIC const MP_DEFINE_STR_OBJ(platform_obj, MICROPY_PY_SYS_PLATFORM);
#endif

// exit([retval]): raise SystemExit, with optional argument given to the exception
STATIC mp_obj_t mp_sys_exit(size_t n_args, const mp_obj_t *args) {
    mp_obj_t exc;
    if (n_args == 0) {
        exc = mp_obj_new_exception(&mp_type_SystemExit);
    } else {
        exc = mp_obj_new_exception_arg1(&mp_type_SystemExit, args[0]);
    }
    nlr_raise(exc);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_sys_exit_obj, 0, 1, mp_sys_exit);

STATIC mp_obj_t mp_sys_print_exception(size_t n_args, const mp_obj_t *args) {
    #if MICROPY_PY_IO && MICROPY_PY_SYS_STDFILES
    void *stream_obj = &mp_sys_stdout_obj;
    if (n_args > 1) {
        mp_get_stream_raise(args[1], MP_STREAM_OP_WRITE);
        stream_obj = MP_OBJ_TO_PTR(args[1]);
    }

    mp_print_t print = {stream_obj, mp_stream_write_adaptor};
    mp_obj_print_exception(&print, args[0]);
    #else
    (void)n_args;
    mp_obj_print_exception(&mp_plat_print, args[0]);
    #endif

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_sys_print_exception_obj, 1, 2, mp_sys_print_exception);

#if MICROPY_PY_SYS_EXC_INFO
STATIC mp_obj_t mp_sys_exc_info(void) {
    mp_obj_t cur_exc = MP_OBJ_FROM_PTR(MP_STATE_VM(cur_exception));
    mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(3, NULL));

    if (cur_exc == MP_OBJ_NULL) {
        t->items[0] = mp_const_none;
        t->items[1] = mp_const_none;
        t->items[2] = mp_const_none;
        return MP_OBJ_FROM_PTR(t);
    }

    t->items[0] = MP_OBJ_FROM_PTR(mp_obj_get_type(cur_exc));
    t->items[1] = cur_exc;
    t->items[2] = mp_const_none;
    return MP_OBJ_FROM_PTR(t);
}
MP_DEFINE_CONST_FUN_OBJ_0(mp_sys_exc_info_obj, mp_sys_exc_info);
#endif

#if MICROPY_PY_SYS_GETSIZEOF
STATIC mp_obj_t mp_sys_getsizeof(mp_obj_t obj) {
    return mp_unary_op(MP_UNARY_OP_SIZEOF, obj);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_sys_getsizeof_obj, mp_sys_getsizeof);
#endif

STATIC const mp_rom_map_elem_t mp_module_sys_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_sys) },

    { MP_ROM_QSTR(MP_QSTR_path), MP_ROM_PTR(&MP_STATE_VM(mp_sys_path_obj)) },
    { MP_ROM_QSTR(MP_QSTR_argv), MP_ROM_PTR(&MP_STATE_VM(mp_sys_argv_obj)) },
    { MP_ROM_QSTR(MP_QSTR_version), MP_ROM_PTR(&version_obj) },
    { MP_ROM_QSTR(MP_QSTR_version_info), MP_ROM_PTR(&mp_sys_version_info_obj) },
    { MP_ROM_QSTR(MP_QSTR_implementation), MP_ROM_PTR(&mp_sys_implementation_obj) },
    #ifdef MICROPY_PY_SYS_PLATFORM
    { MP_ROM_QSTR(MP_QSTR_platform), MP_ROM_PTR(&platform_obj) },
    #endif
    #if MP_ENDIANNESS_LITTLE
    { MP_ROM_QSTR(MP_QSTR_byteorder), MP_ROM_QSTR(MP_QSTR_little) },
    #else
    { MP_ROM_QSTR(MP_QSTR_byteorder), MP_ROM_QSTR(MP_QSTR_big) },
    #endif

    #if MICROPY_PY_SYS_MAXSIZE
    #if MICROPY_LONGINT_IMPL == MICROPY_LONGINT_IMPL_NONE
    // Maximum mp_int_t value is not representable as small int, so we have
    // little choice but to use MP_SMALL_INT_MAX. Apps also should be careful
    // to not try to compare sys.maxsize to some literal number (as this
    // number might not fit in available int size), but instead count number
    // of "one" bits in sys.maxsize.
    { MP_ROM_QSTR(MP_QSTR_maxsize), MP_ROM_INT(MP_SMALL_INT_MAX) },
    #else
    { MP_ROM_QSTR(MP_QSTR_maxsize), MP_ROM_PTR(&mp_maxsize_obj) },
    #endif
    #endif

    #if MICROPY_PY_SYS_EXIT
    { MP_ROM_QSTR(MP_QSTR_exit), MP_ROM_PTR(&mp_sys_exit_obj) },
    #endif

    #if MICROPY_PY_SYS_STDFILES
    { MP_ROM_QSTR(MP_QSTR_stdin), MP_ROM_PTR(&mp_sys_stdin_obj) },
    { MP_ROM_QSTR(MP_QSTR_stdout), MP_ROM_PTR(&mp_sys_stdout_obj) },
    { MP_ROM_QSTR(MP_QSTR_stderr), MP_ROM_PTR(&mp_sys_stderr_obj) },
    #endif

    #if MICROPY_PY_SYS_MODULES
    { MP_ROM_QSTR(MP_QSTR_modules), MP_ROM_PTR(&MP_STATE_VM(mp_loaded_modules_dict)) },
    #endif
    #if MICROPY_PY_SYS_EXC_INFO
    { MP_ROM_QSTR(MP_QSTR_exc_info), MP_ROM_PTR(&mp_sys_exc_info_obj) },
    #endif
    #if MICROPY_PY_SYS_GETSIZEOF
    { MP_ROM_QSTR(MP_QSTR_getsizeof), MP_ROM_PTR(&mp_sys_getsizeof_obj) },
    #endif

    /*
     * Extensions to CPython
     */

    { MP_ROM_QSTR(MP_QSTR_print_exception), MP_ROM_PTR(&mp_sys_print_exception_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_sys_globals, mp_module_sys_globals_table);

const mp_obj_module_t mp_module_sys = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_sys_globals,
};

#endif
