/*
 * virtypedparamtest.c: Test typed param functions
 *
 * Copyright (C) 2015 Mirantis, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 */

#include <config.h>

#include <stdio.h>
#include <virtypedparam.h>

#include "testutils.h"

#define VIR_FROM_THIS VIR_FROM_NONE

typedef struct _TypedParameterTest {
    /* Test name for logging */
    const char          *name;
    /* Flags of the "foobar" parameter check */
    int                  foobar_flags;
    /* Parameters to validate */
    virTypedParameterPtr params;
    /* Amount of parameters */
    int                  nparams;

    /* Expected error code */
    int                  expected_errcode;
    /* Expected error message */
    const char          *expected_errmessage;
} TypedParameterTest;

static int
testTypedParamsValidate(const void *opaque)
{
    int rv;
    TypedParameterTest *test = (TypedParameterTest *)opaque;
    virErrorPtr errptr;

    rv = virTypedParamsValidate(
            test->params, test->nparams,
            "foobar", VIR_TYPED_PARAM_STRING | test->foobar_flags,
            "foo", VIR_TYPED_PARAM_INT,
            "bar", VIR_TYPED_PARAM_UINT,
            NULL);

    if (test->expected_errcode) {
        errptr = virGetLastError();

        rv = (errptr == NULL) || ((rv < 0) &&
                                  !(errptr->code == test->expected_errcode));
        if (errptr && test->expected_errmessage) {
            rv = STRNEQ(test->expected_errmessage, errptr->message);
            if (rv)
                printf("%s\n", errptr->message);
        }
    }

    return rv;
}

#define TEST(testname) {                    \
                        .name = testname,

#define ENDTEST    },

#define FOOBAR_SINGLE   .foobar_flags = 0,
#define FOOBAR_MULTIPLE .foobar_flags = VIR_TYPED_PARAM_MULTIPLE,

#define EXPECTED_OK .expected_errcode = 0, .expected_errmessage = NULL,
#define EXPECTED_ERROR(code, msg) .expected_errcode = code, \
                                  .expected_errmessage = msg,

#define PARAMS_ARRAY(...) ((virTypedParameter[]){ __VA_ARGS__ })
#define PARAMS_SIZE(...) ARRAY_CARDINALITY(PARAMS_ARRAY(__VA_ARGS__))

#define PARAMS(...) \
    .params  = PARAMS_ARRAY(__VA_ARGS__), \
    .nparams = PARAMS_SIZE(__VA_ARGS__),

#define PARAM(field_, type_) { .field = field_, .type = type_ }

static int
testTypedParamsPick(const void *opaque ATTRIBUTE_UNUSED)
{
    int i, rv = -1;
    virTypedParameter params[] = {
        { .field = "bar", .type = VIR_TYPED_PARAM_UINT },
        { .field = "foo", .type = VIR_TYPED_PARAM_INT },
        { .field = "bar", .type = VIR_TYPED_PARAM_UINT },
        { .field = "foo", .type = VIR_TYPED_PARAM_INT },
        { .field = "foobar", .type = VIR_TYPED_PARAM_STRING },
        { .field = "foo", .type = VIR_TYPED_PARAM_INT }
    };
    virTypedParameterPtr *picked_params = NULL;
    size_t picked;


    picked_params = virTypedParamsPick(params, ARRAY_CARDINALITY(params),
                                       "foo", VIR_TYPED_PARAM_INT, &picked);
    if (picked != 3)
        goto cleanup;

    for (i = 0; i < picked; i++) {
        if (picked_params[i] != &params[1 + i * 2])
            goto cleanup;
    }
    VIR_FREE(picked_params);

    picked_params = virTypedParamsPick(params, ARRAY_CARDINALITY(params),
                                       "bar", VIR_TYPED_PARAM_UINT, &picked);

    if (picked != 2)
        goto cleanup;

    for (i = 0; i < picked; i++) {
        if (picked_params[i] != &params[i * 2])
            goto cleanup;
    }

    rv = 0;
 cleanup:
    VIR_FREE(picked_params);
    return rv;
}

static int
testTypedParamsPickStrings(const void *opaque ATTRIBUTE_UNUSED)
{
    int i = 0, rv = -1;
    const char **strings = NULL;
    virTypedParameter params[] = {
        { .field = "bar", .type = VIR_TYPED_PARAM_STRING,
          .value = { .s = (char*)"bar1"} },
        { .field = "foo", .type = VIR_TYPED_PARAM_INT },
        { .field = "bar", .type = VIR_TYPED_PARAM_STRING,
          .value = { .s = (char*)"bar2"} },
        { .field = "foo", .type = VIR_TYPED_PARAM_INT },
        { .field = "foobar", .type = VIR_TYPED_PARAM_STRING },
        { .field = "foo", .type = VIR_TYPED_PARAM_INT },
        { .field = "bar", .type = VIR_TYPED_PARAM_STRING,
          .value = { .s = (char*)"bar3"} }
    };

    strings = virTypedParamsPickStrings(params,
                                        ARRAY_CARDINALITY(params),
                                        "bar");

    for (i = 0; strings[i]; i++) {
        if (!STREQLEN(strings[i], "bar", 3))
            goto cleanup;
        if (strings[i][3] != i + '1')
            goto cleanup;
    }

    rv = 0;
 cleanup:
    VIR_FREE(strings);
    return rv;
}

static int
testTypedParamsValidator(void)
{
    int i, rv = 0;

    TypedParameterTest test[] = {
        TEST("Invalid arg type")
            FOOBAR_SINGLE
            PARAMS( PARAM("foobar", VIR_TYPED_PARAM_INT) )
            EXPECTED_ERROR(
                VIR_ERR_INVALID_ARG,
                "invalid argument: invalid type 'int' for parameter "
                "'foobar', expected 'string'"
            )
        ENDTEST
        TEST("Extra arg")
            FOOBAR_SINGLE
            PARAMS( PARAM("f", VIR_TYPED_PARAM_INT) )
            EXPECTED_ERROR(
                VIR_ERR_INVALID_ARG,
                "argument unsupported: parameter 'f' not supported"
            )
        ENDTEST
        TEST("Valid parameters")
            FOOBAR_SINGLE
            PARAMS(
                PARAM( "bar",  VIR_TYPED_PARAM_UINT ),
                PARAM( "foobar", VIR_TYPED_PARAM_STRING ),
                PARAM( "foo", VIR_TYPED_PARAM_INT )
            )
            EXPECTED_OK
        ENDTEST
        TEST("Duplicates incorrect")
            FOOBAR_SINGLE
            PARAMS(
                PARAM("bar", VIR_TYPED_PARAM_UINT ),
                PARAM("foobar", VIR_TYPED_PARAM_STRING ),
                PARAM("foobar", VIR_TYPED_PARAM_STRING ),
                PARAM("foo", VIR_TYPED_PARAM_INT )
            )
            EXPECTED_ERROR(
                VIR_ERR_INVALID_ARG,
                "invalid argument: parameter 'foobar' occurs multiple times"
            )
        ENDTEST
        TEST("Duplicates OK for marked")
            FOOBAR_MULTIPLE
            PARAMS(
                PARAM("bar", VIR_TYPED_PARAM_UINT ),
                PARAM("foobar", VIR_TYPED_PARAM_STRING ),
                PARAM("foobar", VIR_TYPED_PARAM_STRING ),
                PARAM("foo", VIR_TYPED_PARAM_INT )
            )
            EXPECTED_OK
        },
        TEST(NULL) ENDTEST
    };

    for (i = 0; test[i].name; ++i) {
        if (virtTestRun(test[i].name, testTypedParamsValidate, &test[i]) < 0)
            rv = -1;
    }

    return rv;
}

static int
mymain(void)
{
    int rv = 0;

    if (testTypedParamsValidator() < 0)
        rv = -1;

    if (virtTestRun("Picking", testTypedParamsPick, NULL) < 0)
        rv = -1;

    if (virtTestRun("Picking Strings", testTypedParamsPickStrings, NULL) < 0)
        rv = -1;

    if (rv < 0)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

VIRT_TEST_MAIN(mymain)
