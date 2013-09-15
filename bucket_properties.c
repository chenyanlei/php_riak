/*
   Copyright 2013 Kaspar Bach Pedersen

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include <php.h>
#include <zend_interfaces.h>
#include <ext/spl/spl_iterators.h>
#include <ext/spl/spl_array.h>
#include "bucket_properties.h"
#include "bucket.h"
#include "connection.h"
#include "object.h"
#include "exceptions.h"
#include "php_riak.h"

zend_class_entry *riak_bucket_properties_ce;
zend_class_entry *riak_module_function_ce;
zend_class_entry *riak_commit_hook_ce;
zend_class_entry *riak_commit_hook_list_ce;

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_props_ctor, 0, ZEND_RETURN_VALUE, 2)
    ZEND_ARG_INFO(0, nVal)
    ZEND_ARG_INFO(0, allowMult)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_props_noargs, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_props_set_nval, 0, ZEND_RETURN_VALUE, 1)
    ZEND_ARG_INFO(0, nVal)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_props_set_allow_mult, 0, ZEND_RETURN_VALUE, 1)
    ZEND_ARG_INFO(0, allowMult)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_props_set_last_write_wins, 0, ZEND_RETURN_VALUE, 1)
    ZEND_ARG_INFO(0, lastWriteWins)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_props_set_vclock, 0, ZEND_RETURN_VALUE, 1)
    ZEND_ARG_INFO(0, vClockValue)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_props_set_rwpr, 0, ZEND_RETURN_VALUE, 1)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_module_function_ctor, 0, ZEND_RETURN_VALUE, 2)
    ZEND_ARG_INFO(0, module)
    ZEND_ARG_INFO(0, function)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_module_function_set, 0, ZEND_RETURN_VALUE, 1)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_module_function_noargs, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_commit_hook_ctor, 0, ZEND_RETURN_VALUE, 2)
    ZEND_ARG_INFO(0, name)
    ZEND_ARG_INFO(0, moduleFunction)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_commit_hook_set, 0, ZEND_RETURN_VALUE, 1)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_commit_hook_set_noargs, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_commit_hook_list_offset_exists, 0, ZEND_RETURN_VALUE, 1)
    ZEND_ARG_INFO(0, offset)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_commit_hook_list_offset_set, 0, ZEND_RETURN_VALUE, 2)
    ZEND_ARG_INFO(0, offset)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_commit_hook_list_noargs, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

static zend_function_entry riak_module_function_methods[] = {
    PHP_ME(RiakModuleFunction, __construct, arginfo_module_function_ctor, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_ME(RiakModuleFunction, getModule, arginfo_module_function_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakModuleFunction, setModule, arginfo_module_function_set, ZEND_ACC_PUBLIC)
    PHP_ME(RiakModuleFunction, getFunction, arginfo_module_function_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakModuleFunction, setFunction, arginfo_module_function_set, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

static zend_function_entry riak_commit_hook_methods[] = {
    PHP_ME(RiakCommitHook, __construct, arginfo_commit_hook_ctor, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_ME(RiakCommitHook, getName, arginfo_module_function_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakCommitHook, setName, arginfo_commit_hook_set, ZEND_ACC_PUBLIC)
    PHP_ME(RiakCommitHook, getModuleFunction, arginfo_module_function_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakCommitHook, setModuleFunction, arginfo_commit_hook_set, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

static zend_function_entry riak_commit_hook_list_methods[] = {
    PHP_ME(RiakCommitHookList, __construct, arginfo_commit_hook_list_noargs, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_ME(RiakCommitHookList, offsetExists, arginfo_commit_hook_list_offset_exists, ZEND_ACC_PUBLIC)
    PHP_ME(RiakCommitHookList, offsetGet, arginfo_commit_hook_list_offset_exists, ZEND_ACC_PUBLIC)
    PHP_ME(RiakCommitHookList, offsetSet, arginfo_commit_hook_list_offset_set, ZEND_ACC_PUBLIC)
    PHP_ME(RiakCommitHookList, offsetUnset, arginfo_commit_hook_list_offset_exists, ZEND_ACC_PUBLIC)
    PHP_ME(RiakCommitHookList, count, arginfo_commit_hook_list_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakCommitHookList, getIterator, arginfo_commit_hook_list_noargs, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

static zend_function_entry riak_bucket_properties_methods[] = {
	PHP_ME(RiakBucketProperties, __construct, arginfo_bucket_props_ctor, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_ME(RiakBucketProperties, getNValue, arginfo_bucket_props_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, setNValue, arginfo_bucket_props_set_nval, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, getAllowMult, arginfo_bucket_props_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, setAllowMult, arginfo_bucket_props_set_allow_mult, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, getLastWriteWins, arginfo_bucket_props_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, setLastWriteWins, arginfo_bucket_props_set_last_write_wins, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, getOldVClock, arginfo_bucket_props_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, setOldVClock, arginfo_bucket_props_set_vclock, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, getYoungVClock, arginfo_bucket_props_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, setYoungVClock, arginfo_bucket_props_set_vclock, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, getSmallVClock, arginfo_bucket_props_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, setSmallVClock, arginfo_bucket_props_set_vclock, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, getBigVClock, arginfo_bucket_props_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, setBigVClock, arginfo_bucket_props_set_vclock, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, getR, arginfo_bucket_props_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, setR, arginfo_bucket_props_set_rwpr, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, getPR, arginfo_bucket_props_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, setPR, arginfo_bucket_props_set_rwpr, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, getW, arginfo_bucket_props_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, setW, arginfo_bucket_props_set_rwpr, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, getDW, arginfo_bucket_props_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, setDW, arginfo_bucket_props_set_rwpr, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, getPW, arginfo_bucket_props_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, setPW, arginfo_bucket_props_set_rwpr, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, getRW, arginfo_bucket_props_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, setRW, arginfo_bucket_props_set_rwpr, ZEND_ACC_PUBLIC)

    PHP_ME(RiakBucketProperties, getBasicQuorum, arginfo_bucket_props_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, setBasicQuorum, arginfo_bucket_props_set_rwpr, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, getNotFoundOk, arginfo_bucket_props_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, setNotFoundOk, arginfo_bucket_props_set_rwpr, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, getSearchEnabled, arginfo_bucket_props_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, setSearchEnabled, arginfo_bucket_props_set_rwpr, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, getBackend, arginfo_bucket_props_noargs, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucketProperties, setBackend, arginfo_bucket_props_set_rwpr, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

void riak_bucket_props_init(TSRMLS_D)/* {{{ */
{
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, "Riak", "BucketPropertyList", riak_bucket_properties_methods);
	riak_bucket_properties_ce = zend_register_internal_class(&ce TSRMLS_CC);

    zend_declare_property_long(riak_bucket_properties_ce, "nVal", sizeof("nVal")-1, 3, ZEND_ACC_PRIVATE TSRMLS_CC);
    zend_declare_property_bool(riak_bucket_properties_ce, "allowMult", sizeof("allowMult")-1, 0, ZEND_ACC_PRIVATE TSRMLS_CC);

    zend_declare_property_null(riak_bucket_properties_ce, "lastWriteWins", sizeof("lastWriteWins")-1, ZEND_ACC_PRIVATE TSRMLS_CC);
    zend_declare_property_null(riak_bucket_properties_ce, "oldVClock", sizeof("oldVClock")-1, ZEND_ACC_PRIVATE TSRMLS_CC);
    zend_declare_property_null(riak_bucket_properties_ce, "youngVClock", sizeof("youngVClock")-1, ZEND_ACC_PRIVATE TSRMLS_CC);
    zend_declare_property_null(riak_bucket_properties_ce, "smallVClock", sizeof("smallVClock")-1, ZEND_ACC_PRIVATE TSRMLS_CC);
    zend_declare_property_null(riak_bucket_properties_ce, "bigVClock", sizeof("bigVClock")-1, ZEND_ACC_PRIVATE TSRMLS_CC);
    zend_declare_property_null(riak_bucket_properties_ce, "r", sizeof("r")-1, ZEND_ACC_PRIVATE TSRMLS_CC);
    zend_declare_property_null(riak_bucket_properties_ce, "pr", sizeof("pr")-1, ZEND_ACC_PRIVATE TSRMLS_CC);
    zend_declare_property_null(riak_bucket_properties_ce, "w", sizeof("w")-1, ZEND_ACC_PRIVATE TSRMLS_CC);
    zend_declare_property_null(riak_bucket_properties_ce, "dw", sizeof("dw")-1, ZEND_ACC_PRIVATE TSRMLS_CC);
    zend_declare_property_null(riak_bucket_properties_ce, "pw", sizeof("pw")-1, ZEND_ACC_PRIVATE TSRMLS_CC);
    zend_declare_property_null(riak_bucket_properties_ce, "rw", sizeof("rw")-1, ZEND_ACC_PRIVATE TSRMLS_CC);
    zend_declare_property_null(riak_bucket_properties_ce, "basicQuorum", sizeof("basicQuorum")-1, ZEND_ACC_PRIVATE TSRMLS_CC);
    zend_declare_property_null(riak_bucket_properties_ce, "notFoundOk", sizeof("notFoundOk")-1, ZEND_ACC_PRIVATE TSRMLS_CC);
    zend_declare_property_null(riak_bucket_properties_ce, "searchEnabled", sizeof("searchEnabled")-1, ZEND_ACC_PRIVATE TSRMLS_CC);
    zend_declare_property_null(riak_bucket_properties_ce, "backend", sizeof("backend")-1, ZEND_ACC_PRIVATE TSRMLS_CC);

    //zend_declare_property_null(riak_bucket_properties_ce, "precommit_hooks", sizeof("precommit_hooks")-1, ZEND_ACC_PRIVATE TSRMLS_CC);

    INIT_NS_CLASS_ENTRY(ce, "Riak\\Property", "ModuleFunction", riak_module_function_methods);
    riak_module_function_ce = zend_register_internal_class(&ce TSRMLS_CC);
    zend_declare_property_null(riak_bucket_properties_ce, "module", sizeof("module")-1, ZEND_ACC_PRIVATE TSRMLS_CC);
    zend_declare_property_null(riak_bucket_properties_ce, "function", sizeof("function")-1, ZEND_ACC_PRIVATE TSRMLS_CC);

    INIT_NS_CLASS_ENTRY(ce, "Riak\\Property", "CommitHook", riak_commit_hook_methods);
    riak_commit_hook_ce = zend_register_internal_class(&ce TSRMLS_CC);
    zend_declare_property_null(riak_commit_hook_ce, "name", sizeof("name")-1, ZEND_ACC_PRIVATE TSRMLS_CC);
    zend_declare_property_null(riak_commit_hook_ce, "moduleFunction", sizeof("moduleFunction")-1, ZEND_ACC_PRIVATE TSRMLS_CC);

    INIT_NS_CLASS_ENTRY(ce, "Riak\\Property", "CommitHookList", riak_commit_hook_list_methods);
    riak_commit_hook_list_ce = zend_register_internal_class(&ce TSRMLS_CC);
    zend_class_implements(riak_commit_hook_list_ce TSRMLS_CC, 3, spl_ce_ArrayAccess, spl_ce_Aggregate, spl_ce_Countable);
    zend_declare_property_null(riak_commit_hook_list_ce, "hooks", sizeof("hooks")-1, ZEND_ACC_PRIVATE TSRMLS_CC);
}
/* }}} */

/* {{{ proto void Riak\Property\RiakCommitHookList->__construct()
Creates a new Riak\Property\RiakCommitHookList */
PHP_METHOD(RiakCommitHookList, __construct)
{
    zval* zhooks;
    // Start with an empty array
    MAKE_STD_ZVAL(zhooks);
    object_init_ex(zhooks, spl_ce_ArrayObject);
    zend_update_property(riak_commit_hook_list_ce, getThis(), "hooks", sizeof("hooks")-1, zhooks TSRMLS_CC);
    zval_ptr_dtor(&zhooks);
}/* }}} */

PHP_METHOD(RiakCommitHookList, offsetExists)
{
    zval *zoffset, *zhooks, *zresult;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zoffset) == FAILURE) {
        return;
    }
    zhooks = zend_read_property(riak_commit_hook_list_ce, getThis(), "hooks", sizeof("hooks")-1, 1 TSRMLS_CC);
    zend_call_method_with_1_params(&zhooks, spl_ce_ArrayObject, NULL, "offsetexists", &zresult, zoffset);
    RETURN_ZVAL(zresult, 0, 1);
}

PHP_METHOD(RiakCommitHookList, offsetGet)
{
    zval *zoffset, *zhooks, *zresult;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zoffset) == FAILURE) {
        return;
    }
    zhooks = zend_read_property(riak_commit_hook_list_ce, getThis(), "hooks", sizeof("hooks")-1, 1 TSRMLS_CC);
    zend_call_method_with_1_params(&zhooks, spl_ce_ArrayObject, NULL, "offsetget", &zresult, zoffset);
    RETURN_ZVAL(zresult, 0, 1);
}

PHP_METHOD(RiakCommitHookList, offsetSet)
{
    zval *zoffset, *zvalue, *zhooks;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zO", &zoffset, &zvalue, riak_commit_hook_ce) == FAILURE) {
        return;
    }
    zhooks = zend_read_property(riak_commit_hook_list_ce, getThis(), "hooks", sizeof("hooks")-1, 1 TSRMLS_CC);
    zend_call_method_with_2_params(&zhooks, spl_ce_ArrayObject, NULL, "offsetset", NULL, zoffset, zvalue);
}

PHP_METHOD(RiakCommitHookList, offsetUnset)
{
    zval *zoffset, *zhooks;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zoffset) == FAILURE) {
        return;
    }
    zhooks = zend_read_property(riak_commit_hook_list_ce, getThis(), "hooks", sizeof("hooks")-1, 1 TSRMLS_CC);
    zend_call_method_with_1_params(&zhooks, spl_ce_ArrayObject, NULL, "offsetunset", NULL, zoffset);
}

PHP_METHOD(RiakCommitHookList, count)
{
    zval *zoffset, *zhooks, *zresult;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zoffset) == FAILURE) {
        return;
    }
    zhooks = zend_read_property(riak_commit_hook_list_ce, getThis(), "hooks", sizeof("hooks")-1, 1 TSRMLS_CC);
    zend_call_method_with_0_params(&zhooks, spl_ce_ArrayObject, NULL, "count", &zresult);
    RETURN_ZVAL(zresult, 0, 1);
}

PHP_METHOD(RiakCommitHookList, getIterator)
{
    // TODO Create and return a new array iterator
}

/* {{{ proto void Riak\Property\ModuleFunction->__construct(string $module, string $function)
Creates a new Riak\Property\ModuleFunction */
PHP_METHOD(RiakModuleFunction, __construct)
{
    char *module, *efunction;
    int module_len, efunction_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &module, &module_len, &efunction, &efunction_len) == FAILURE) {
        return;
    }

    zend_update_property_stringl(riak_module_function_ce, getThis(), "module", sizeof("module")-1, module, module_len TSRMLS_CC);
    zend_update_property_stringl(riak_module_function_ce, getThis(), "function", sizeof("function")-1, efunction, efunction_len TSRMLS_CC);
}
/* }}} */

PHP_METHOD(RiakModuleFunction, getModule)
{
    RIAK_GETTER_STRING(riak_bucket_properties_ce, "module")
}

PHP_METHOD(RiakModuleFunction, setModule)
{
    RIAK_SETTER_STRING(riak_bucket_properties_ce, "module")
    RIAK_RETURN_THIS
}

PHP_METHOD(RiakModuleFunction, getFunction)
{
    RIAK_GETTER_STRING(riak_bucket_properties_ce, "function")
}

PHP_METHOD(RiakModuleFunction, setFunction)
{
    RIAK_SETTER_STRING(riak_bucket_properties_ce, "function")
    RIAK_RETURN_THIS
}

/* {{{ proto void Riak\Property\CommitHook->__construct(string $name, ModuleFunction $moduleFunction)
Creates a new Riak\Property\CommitHook */
PHP_METHOD(RiakCommitHook, __construct)
{
    char* name;
    int name_len;
    zval *zmodule_function;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sO", &name, &name_len, &zmodule_function, riak_module_function_ce) == FAILURE) {
        return;
    }
    zend_update_property_stringl(riak_commit_hook_ce, getThis(), "name", sizeof("name")-1, name, name_len TSRMLS_CC);
    zend_update_property(riak_commit_hook_ce, getThis(), "moduleFunction", sizeof("moduleFunction")-1, zmodule_function TSRMLS_CC);
}
/* }}} */

PHP_METHOD(RiakCommitHook, getName)
{
    RIAK_GETTER_STRING(riak_commit_hook_ce, "name")
}

PHP_METHOD(RiakCommitHook, setName)
{
    RIAK_SETTER_STRING(riak_commit_hook_ce, "name")
    RIAK_RETURN_THIS
}

PHP_METHOD(RiakCommitHook, getModuleFunction)
{
    RIAK_GETTER_STRING(riak_commit_hook_ce, "moduleFunction")
}

PHP_METHOD(RiakCommitHook, setModuleFunction)
{
    RIAK_SETTER_STRING(riak_commit_hook_ce, "moduleFunction")
    RIAK_RETURN_THIS
}


/* {{{ proto void Riak\BucketProperties->__construct(int $nVal, bool $allowMult)
Creates a new RiakBucketProperties */
PHP_METHOD(RiakBucketProperties, __construct)
{
	long nVal;
	zend_bool allowMult;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lb", &nVal, &allowMult) == FAILURE) {
		return;
	}
	zend_update_property_long(riak_bucket_properties_ce, getThis(), "nVal", sizeof("nVal")-1, nVal TSRMLS_CC);
	zend_update_property_bool(riak_bucket_properties_ce, getThis(), "allowMult", sizeof("allowMult")-1, allowMult TSRMLS_CC);
}
/* }}} */

PHP_METHOD(RiakBucketProperties, getNValue)
{
    RIAK_GETTER_LONG(riak_bucket_properties_ce, "nVal")
}

PHP_METHOD(RiakBucketProperties, setNValue)
{
    RIAK_SETTER_LONG(riak_bucket_properties_ce, "nVal")
    RIAK_RETURN_THIS
}

PHP_METHOD(RiakBucketProperties, getAllowMult)
{
    RIAK_GETTER_BOOL(riak_bucket_properties_ce, "allowMult")
}

PHP_METHOD(RiakBucketProperties, setAllowMult)
{
    RIAK_SETTER_BOOL(riak_bucket_properties_ce, "allowMult")
    RIAK_RETURN_THIS
}

PHP_METHOD(RiakBucketProperties, getLastWriteWins)
{
    RIAK_GETTER_BOOL(riak_bucket_properties_ce, "lastWriteWins")
}

PHP_METHOD(RiakBucketProperties, setLastWriteWins)
{
    RIAK_SETTER_BOOL(riak_bucket_properties_ce, "lastWriteWins")
    RIAK_RETURN_THIS
}

PHP_METHOD(RiakBucketProperties, getOldVClock)
{
    RIAK_GETTER_LONG(riak_bucket_properties_ce, "oldVClock")
}

PHP_METHOD(RiakBucketProperties, setOldVClock)
{
    RIAK_SETTER_LONG(riak_bucket_properties_ce, "oldVClock")
    RIAK_RETURN_THIS
}

PHP_METHOD(RiakBucketProperties, getYoungVClock)
{
    RIAK_GETTER_LONG(riak_bucket_properties_ce, "youngVClock")
}

PHP_METHOD(RiakBucketProperties, setYoungVClock)
{
    RIAK_SETTER_LONG(riak_bucket_properties_ce, "youngVClock")
    RIAK_RETURN_THIS
}

PHP_METHOD(RiakBucketProperties, getSmallVClock)
{
    RIAK_GETTER_LONG(riak_bucket_properties_ce, "smallVClock")
}

PHP_METHOD(RiakBucketProperties, setSmallVClock)
{
    RIAK_SETTER_LONG(riak_bucket_properties_ce, "smallVClock")
    RIAK_RETURN_THIS
}

PHP_METHOD(RiakBucketProperties, getBigVClock)
{
    RIAK_GETTER_LONG(riak_bucket_properties_ce, "bigVClock")
}

PHP_METHOD(RiakBucketProperties, setBigVClock)
{
    RIAK_SETTER_LONG(riak_bucket_properties_ce, "bigVClock")
    RIAK_RETURN_THIS
}

PHP_METHOD(RiakBucketProperties, getR)
{
    RIAK_GETTER_LONG(riak_bucket_properties_ce, "r")
}

PHP_METHOD(RiakBucketProperties, setR)
{
    RIAK_SETTER_LONG(riak_bucket_properties_ce, "r")
    RIAK_RETURN_THIS
}

PHP_METHOD(RiakBucketProperties, getPR)
{
    RIAK_GETTER_LONG(riak_bucket_properties_ce, "pr")
}

PHP_METHOD(RiakBucketProperties, setPR)
{
    RIAK_SETTER_LONG(riak_bucket_properties_ce, "pr")
    RIAK_RETURN_THIS
}

PHP_METHOD(RiakBucketProperties, getW)
{
    RIAK_GETTER_LONG(riak_bucket_properties_ce, "w")
}

PHP_METHOD(RiakBucketProperties, setW)
{
    RIAK_SETTER_LONG(riak_bucket_properties_ce, "w")
    RIAK_RETURN_THIS
}

PHP_METHOD(RiakBucketProperties, getDW)
{
    RIAK_GETTER_LONG(riak_bucket_properties_ce, "dw")
}

PHP_METHOD(RiakBucketProperties, setDW)
{
    RIAK_SETTER_LONG(riak_bucket_properties_ce, "dw")
    RIAK_RETURN_THIS
}

PHP_METHOD(RiakBucketProperties, getPW)
{
    RIAK_GETTER_LONG(riak_bucket_properties_ce, "pw")
}

PHP_METHOD(RiakBucketProperties, setPW)
{
    RIAK_SETTER_LONG(riak_bucket_properties_ce, "pw")
    RIAK_RETURN_THIS
}

PHP_METHOD(RiakBucketProperties, getRW)
{
    RIAK_GETTER_LONG(riak_bucket_properties_ce, "rw")
}

PHP_METHOD(RiakBucketProperties, setRW)
{
    RIAK_SETTER_LONG(riak_bucket_properties_ce, "rw")
    RIAK_RETURN_THIS
}

PHP_METHOD(RiakBucketProperties, getBasicQuorum)
{
    RIAK_GETTER_BOOL(riak_bucket_properties_ce, "basicQuorum")
}

PHP_METHOD(RiakBucketProperties, setBasicQuorum)
{
    RIAK_SETTER_BOOL(riak_bucket_properties_ce, "basicQuorum")
    RIAK_RETURN_THIS
}

PHP_METHOD(RiakBucketProperties, getNotFoundOk)
{
    RIAK_GETTER_BOOL(riak_bucket_properties_ce, "notFoundOk")
}

PHP_METHOD(RiakBucketProperties, setNotFoundOk)
{
    RIAK_SETTER_BOOL(riak_bucket_properties_ce, "notFoundOk")
    RIAK_RETURN_THIS
}

PHP_METHOD(RiakBucketProperties, getSearchEnabled)
{
    RIAK_GETTER_BOOL(riak_bucket_properties_ce, "searchEnabled")
}

PHP_METHOD(RiakBucketProperties, setSearchEnabled)
{
    RIAK_SETTER_BOOL(riak_bucket_properties_ce, "searchEnabled")
    RIAK_RETURN_THIS
}

PHP_METHOD(RiakBucketProperties, getBackend)
{
    RIAK_GETTER_STRING(riak_bucket_properties_ce, "backend")
}

PHP_METHOD(RiakBucketProperties, setBackend)
{
    RIAK_SETTER_STRING(riak_bucket_properties_ce, "backend")
    RIAK_RETURN_THIS
}
