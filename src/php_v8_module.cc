/*
 * This file is part of the pinepain/php-v8 PHP extension.
 *
 * Copyright (c) 2015-2017 Bogdan Padalko <pinepain@gmail.com>
 *
 * Licensed under the MIT license: http://opensource.org/licenses/MIT
 *
 * For the full copyright and license information, please view the
 * LICENSE file that was distributed with this source or visit
 * http://opensource.org/licenses/MIT
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_v8_module.h"
#include "php_v8_string.h"
#include "php_v8_module_resolver.h"
#include "php_v8_value.h"
#include "php_v8_isolate_limits.h"
#include "php_v8.h"
#include "zend_interfaces.h"

zend_class_entry* php_v8_module_class_entry;
#define this_ce php_v8_module_class_entry

#define PHP_V8_DEBUG_EXECUTION 1
#ifdef PHP_V8_DEBUG_EXECUTION
#define php_v8_debug_execution(format, ...) fprintf(stderr, (format), ##__VA_ARGS__);
#else
#define php_v8_debug_execution(format, ...)
#endif


static zend_object_handlers php_v8_module_object_handlers;


v8::Local<v8::Module> php_v8_module_get_local(v8::Isolate *isolate, php_v8_module_t *php_v8_module) {
    return v8::Local<v8::Module>::New(isolate, *php_v8_module->persistent);
}

php_v8_module_t * php_v8_module_fetch_object(zend_object *obj) {
    return (php_v8_module_t *)((char *)obj - XtOffsetOf(php_v8_module_t, std));
}

php_v8_module_t * php_v8_create_module(zval *return_value, php_v8_context_t *php_v8_context, v8::Local<v8::Module> module) {
    assert(!module.IsEmpty());

    object_init_ex(return_value, this_ce);

    PHP_V8_FETCH_MODULE_INTO(return_value, php_v8_module);

    PHP_V8_MODULE_STORE_ISOLATE(return_value, &php_v8_context->php_v8_isolate->this_ptr);
    PHP_V8_STORE_POINTER_TO_ISOLATE(php_v8_module, php_v8_context->php_v8_isolate);
    PHP_V8_MODULE_STORE_CONTEXT(return_value, &php_v8_context->this_ptr);
    PHP_V8_STORE_POINTER_TO_CONTEXT(php_v8_module, php_v8_context);

    ZVAL_COPY_VALUE(&php_v8_module->this_ptr, return_value);

    php_v8_module->persistent->Reset(php_v8_context->php_v8_isolate->isolate, module);
    php_v8_module->hash = module->GetIdentityHash();

    auto it = php_v8_context->php_v8_isolate->modules->find(php_v8_module->hash);

    if (it != php_v8_context->php_v8_isolate->modules->end()) {
        // highly unlikely
        PHP_V8_THROW_EXCEPTION("Modules hash collision detected");
        return php_v8_module;
    }

    php_v8_context->php_v8_isolate->modules->insert(std::pair<int, php_v8_module_t *>(php_v8_module->hash, php_v8_module));

    return php_v8_module;
}


static v8::MaybeLocal<v8::Module> php_v8_module_resolve_callback(v8::Local<v8::Context> context, v8::Local<v8::String> specifier, v8::Local<v8::Module> referrer) {
    php_v8_debug_execution("Calling module resolver callback:\n");

    php_v8_context_t *php_v8_context = php_v8_context_get_reference(context);
    assert(php_v8_context->modules->size() > 0);
    php_v8_debug_execution("    module resolver callbacks stack: %lu\n", php_v8_context->php_v8_isolate->module_resolvers->size());

    PHP_V8_DECLARE_ISOLATE(php_v8_context->php_v8_isolate);

    // TODO: assert isolate entered, isolate has context

    zval *module_resolver_zv = php_v8_context->php_v8_isolate->module_resolvers->top();
    auto it = php_v8_context->php_v8_isolate->modules->find(referrer->GetIdentityHash());

    assert(it != php_v8_context->modules->end());

    if (it == php_v8_context->php_v8_isolate->modules->end()) {
        // highly unlikely
        return v8::Local<v8::Module>();
    }

    php_v8_module_t * php_v8_module = it->second;

    zval retval, fname, params[3];

    ZVAL_COPY(&params[0], &php_v8_context->this_ptr);
    php_v8_get_or_create_value(&params[1], specifier, isolate);
    ZVAL_COPY(&params[2], &php_v8_module->this_ptr);

//
////    zend_call_method(module_resolver_zv,
////                     php_v8_module_resolver_class_entry,
////                     zend_hash_str_find_ptr(&php_v8_module_resolver_class_entry->function_table, ZEND_STRL("resolve")),
////                     ZEND_STRL("resolve"),
////                     rv,
////                     3,
////                     arg1,
////                     arg2,
////                     arg3);
//
    ZVAL_STRING(&fname, "resolve");
//
    bool failed = false;
    if (FAILURE == call_user_function(EG(function_table), module_resolver_zv, &fname, &retval, 3, params) || Z_TYPE(retval) == IS_UNDEF) {
        failed = true;

        if (!EG(exception)) {
            zend_throw_exception_ex(NULL, 0, "Failed calling %s::resolve()", ZSTR_VAL(php_v8_module_class_entry->name));
        }

////        if (options & PHP_JSON_PARTIAL_OUTPUT_ON_ERROR) {
////            smart_str_appendl(buf, "null", 4);
////        }
    }
//
    zval_ptr_dtor(&fname);
    zval_ptr_dtor(&params[0]);
    zval_ptr_dtor(&params[1]);
    zval_ptr_dtor(&params[2]);

    if (failed) {
        return v8::Local<v8::Module>();
    }

    PHP_V8_FETCH_MODULE_WITH_CHECK_RET(&retval, php_v8_resolved_module, v8::Local<v8::Module>());

    v8::Local<v8::Module> resolved_module = php_v8_module_get_local(isolate, php_v8_resolved_module);

    // TODO: do we need to cleanup retval
    return resolved_module;
}


static void php_v8_module_free(zend_object *object)
{
    php_v8_module_t *php_v8_module = php_v8_module_fetch_object(object);

    if (php_v8_module->persistent) {
        if (PHP_V8_ISOLATE_HAS_VALID_HANDLE(php_v8_module)) {
            php_v8_module->persistent->Reset();
        }

        delete php_v8_module->persistent;
    }

    auto it = php_v8_module->php_v8_isolate->modules->find(php_v8_module->hash);

    if (it != php_v8_module->php_v8_isolate->modules->end()) {
        // highly likely
        php_v8_debug_execution("erasing module %d\n", php_v8_module->hash);
        php_v8_module->php_v8_isolate->modules->erase(it);
    }

    zend_object_std_dtor(&php_v8_module->std);
}

static zend_object * php_v8_module_ctor(zend_class_entry *ce)
{
    php_v8_module_t *php_v8_module;

    php_v8_module = (php_v8_module_t *) ecalloc(1, sizeof(php_v8_module_t) + zend_object_properties_size(ce));

    zend_object_std_init(&php_v8_module->std, ce);
    object_properties_init(&php_v8_module->std, ce);

    php_v8_module->persistent = new v8::Persistent<v8::Module>();

    php_v8_module->std.handlers = &php_v8_module_object_handlers;

    return &php_v8_module->std;
}

static PHP_METHOD(V8Module, __construct)
{
    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    PHP_V8_THROW_EXCEPTION("V8\\Module::__construct() should not be called. Use other methods which yield V8\\Module object.")
}

static PHP_METHOD(V8Module, GetIsolate)
{
    zval rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    PHP_V8_FETCH_MODULE_WITH_CHECK(getThis(), php_v8_module);

    RETVAL_ZVAL(zend_read_property(this_ce, getThis(), ZEND_STRL("isolate"), 0, &rv), 1, 0);
}

static PHP_METHOD(V8Module, GetContext)
{
    zval rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    PHP_V8_FETCH_MODULE_WITH_CHECK(getThis(), php_v8_module);

    RETVAL_ZVAL(zend_read_property(this_ce, getThis(), ZEND_STRL("context"), 0, &rv), 1, 0);
}

static PHP_METHOD(V8Module, GetModuleRequests)
{
    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    PHP_V8_FETCH_MODULE_WITH_CHECK(getThis(), php_v8_module);
    PHP_V8_ENTER_STORED_ISOLATE(php_v8_module);

    v8::Local<v8::Module> local_module = php_v8_module_get_local(isolate, php_v8_module);

    array_init_size(return_value, static_cast<uint32_t>(local_module->GetModuleRequestsLength()));

    zval tmp;

    for (int i = 0; i > local_module->GetModuleRequestsLength(); i++) {
        php_v8_get_or_create_value(&tmp, local_module->GetModuleRequest(i), isolate);
        add_index_zval(return_value, static_cast<uint32_t>(i), &tmp);
    }
}

static PHP_METHOD(V8Module, GetIdentityHash)
{
    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    PHP_V8_FETCH_MODULE_WITH_CHECK(getThis(), php_v8_module);
    PHP_V8_ENTER_STORED_ISOLATE(php_v8_module);

    v8::Local<v8::Module> local_module = php_v8_module_get_local(isolate, php_v8_module);

    RETURN_LONG(static_cast<zend_long>(local_module->GetIdentityHash()));
}



static PHP_METHOD(V8Module, Instantiate)
{
    zval *php_v8_context_zv;
    zval *php_v8_resolver_zv;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "oo", &php_v8_context_zv, &php_v8_resolver_zv) == FAILURE) {
        return;
    }

    PHP_V8_FETCH_MODULE_WITH_CHECK(getThis(), php_v8_module);
    PHP_V8_CONTEXT_FETCH_WITH_CHECK(php_v8_context_zv, php_v8_context);

    PHP_V8_DATA_ISOLATES_CHECK(php_v8_module, php_v8_context);

    PHP_V8_ENTER_STORED_ISOLATE(php_v8_context);
    PHP_V8_ENTER_CONTEXT(php_v8_context);

    // TODO: try-catch (look into v8 sources for details
    // TODO: wrap interfaced resolver method call into C++ callback function (hint: there could be only single root module which needs to be instantiated?)

    v8::Local<v8::Module> local_module = php_v8_module_get_local(isolate, php_v8_module);

    // TODO: check that it has not been instantiated yet (in this context??)

    PHP_V8_TRY_CATCH(isolate);
    PHP_V8_INIT_ISOLATE_LIMITS_ON_CONTEXT(php_v8_context);

    php_v8_context->php_v8_isolate->module_resolvers->push(php_v8_resolver_zv);
    bool instantiated = local_module->Instantiate(context, php_v8_module_resolve_callback);
    php_v8_context->php_v8_isolate->module_resolvers->pop();

    // TODO: store whether we instantiated or not
    // TODO: can we be instantiated in different contexts at the same time? Isolates?
    PHP_V8_MAYBE_CATCH(php_v8_context, try_catch);

    if (!instantiated) {
        PHP_V8_THROW_EXCEPTION("Failed to instantiate module");
        return;
    }

    RETVAL_BOOL(static_cast<zend_bool>(instantiated));
}

static PHP_METHOD(V8Module, Evaluate)
{
    zval *php_v8_context_zv;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &php_v8_context_zv) == FAILURE) {
        return;
    }

    PHP_V8_FETCH_MODULE_WITH_CHECK(getThis(), php_v8_module);
    PHP_V8_CONTEXT_FETCH_WITH_CHECK(php_v8_context_zv, php_v8_context);

    PHP_V8_DATA_ISOLATES_CHECK(php_v8_module, php_v8_context);

    PHP_V8_ENTER_STORED_ISOLATE(php_v8_module);
    PHP_V8_ENTER_CONTEXT(php_v8_context);

    v8::Local<v8::Module> local_module = php_v8_module_get_local(isolate, php_v8_module);
//    // It's an API error to call Evaluate before Instantiate.
//    CHECK(self->instantiated());

    PHP_V8_TRY_CATCH(isolate);
    PHP_V8_INIT_ISOLATE_LIMITS_ON_CONTEXT(php_v8_context);

    v8::MaybeLocal<v8::Value> maybe_result = local_module->Evaluate(context);

    PHP_V8_MAYBE_CATCH(php_v8_context, try_catch);
    PHP_V8_THROW_VALUE_EXCEPTION_WHEN_EMPTY(maybe_result, "Failed to evaluate module");

    v8::Local<v8::Value> local_result = maybe_result.ToLocalChecked();

    php_v8_get_or_create_value(return_value, local_result, isolate);
}


ZEND_BEGIN_ARG_INFO_EX(arginfo_v8_module___construct, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

PHP_V8_ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_v8_module_GetModuleRequests, ZEND_RETURN_VALUE, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

PHP_V8_ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_v8_module_GetIdentityHash, ZEND_RETURN_VALUE, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

// TODO: we throw on failure, so no return type needed at all
PHP_V8_ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_v8_module_Instantiate, ZEND_RETURN_VALUE, 2, _IS_BOOL, 0)
                ZEND_ARG_OBJ_INFO(0, context, V8\\Context, 0)
                ZEND_ARG_OBJ_INFO(0, resolver, V8\\ModuleResolverInterface, 0)
ZEND_END_ARG_INFO()

PHP_V8_ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_v8_module_Evaluate, ZEND_RETURN_VALUE, 1, _IS_BOOL, 0)
                ZEND_ARG_OBJ_INFO(0, context, V8\\Context, 0)
ZEND_END_ARG_INFO()


static const zend_function_entry php_v8_module_methods[] = {
    PHP_ME(V8Module, __construct,       arginfo_v8_module___construct,          ZEND_ACC_PRIVATE | ZEND_ACC_CTOR)
    PHP_ME(V8Module, GetModuleRequests, arginfo_v8_module_GetModuleRequests,    ZEND_ACC_PUBLIC)
    PHP_ME(V8Module, GetIdentityHash,   arginfo_v8_module_GetIdentityHash,      ZEND_ACC_PUBLIC)
    PHP_ME(V8Module, Instantiate,       arginfo_v8_module_Instantiate,          ZEND_ACC_PUBLIC)
    PHP_ME(V8Module, Evaluate,          arginfo_v8_module_Evaluate,             ZEND_ACC_PUBLIC)

    PHP_FE_END
};


PHP_MINIT_FUNCTION(php_v8_module)
{
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, PHP_V8_NS, "Module", php_v8_module_methods);
    this_ce = zend_register_internal_class(&ce);
    this_ce->create_object = php_v8_module_ctor;

    zend_declare_property_null(this_ce, ZEND_STRL("isolate"), ZEND_ACC_PRIVATE);
    zend_declare_property_null(this_ce, ZEND_STRL("context"), ZEND_ACC_PRIVATE);

    memcpy(&php_v8_module_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

    php_v8_module_object_handlers.offset = XtOffsetOf(php_v8_module_t, std);
    php_v8_module_object_handlers.free_obj = php_v8_module_free;

    return SUCCESS;
}
