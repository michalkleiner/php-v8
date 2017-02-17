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

#ifndef PHP_V8_MODULE_H
#define PHP_V8_MODULE_H

typedef struct _php_v8_module_t php_v8_module_t;

#include "php_v8_exceptions.h"
#include "php_v8_isolate.h"
#include <v8.h>

extern "C" {
#include "php.h"

#ifdef ZTS
#include "TSRM.h"
#endif
}

extern zend_class_entry *php_v8_module_class_entry;

extern v8::Local<v8::Module> php_v8_module_get_local(v8::Isolate *isolate, php_v8_module_t *php_v8_module);
extern php_v8_module_t * php_v8_module_fetch_object(zend_object *obj);
extern php_v8_module_t * php_v8_create_module(zval *return_value, php_v8_context_t *php_v8_context, v8::Local<v8::Module> module);


#define PHP_V8_FETCH_MODULE(zv) php_v8_module_fetch_object(Z_OBJ_P(zv))
#define PHP_V8_FETCH_MODULE_INTO(pzval, into) php_v8_module_t *(into) = PHP_V8_FETCH_MODULE((pzval))

#define PHP_V8_EMPTY_MODULE_MSG "Module" PHP_V8_EMPTY_HANDLER_MSG_PART
#define PHP_V8_CHECK_EMPTY_MODULE_HANDLER(val) PHP_V8_CHECK_EMPTY_HANDLER((val), PHP_V8_EMPTY_MODULE_MSG)
#define PHP_V8_CHECK_EMPTY_MODULE_HANDLER_RET(val, ret) PHP_V8_CHECK_EMPTY_HANDLER_RET((val), PHP_V8_EMPTY_MODULE_MSG, (ret))

#define PHP_V8_FETCH_MODULE_WITH_CHECK(pzval, into) \
    PHP_V8_FETCH_MODULE_INTO(pzval, into); \
    PHP_V8_CHECK_EMPTY_MODULE_HANDLER(into);

#define PHP_V8_FETCH_MODULE_WITH_CHECK_RET(pzval, into, ret) \
    PHP_V8_FETCH_MODULE_INTO(pzval, into); \
    PHP_V8_CHECK_EMPTY_MODULE_HANDLER_RET(into, ret);


#define PHP_V8_MODULE_STORE_ISOLATE(to_zval, isolate_zv) zend_update_property(php_v8_module_class_entry, (to_zval), ZEND_STRL("isolate"), (isolate_zv));
#define PHP_V8_MODULE_READ_ISOLATE(from_zval) zend_read_property(php_v8_module_class_entry, (from_zval), ZEND_STRL("isolate"), 0, &rv)

#define PHP_V8_MODULE_STORE_CONTEXT(to_zval, isolate_zv) zend_update_property(php_v8_module_class_entry, (to_zval), ZEND_STRL("context"), (isolate_zv));
#define PHP_V8_MODULE_READ_CONTEXT(from_zval) zend_read_property(php_v8_module_class_entry, (from_zval), ZEND_STRL("context"), 0, &rv)


struct _php_v8_module_t {
    php_v8_isolate_t *php_v8_isolate;
    php_v8_context_t *php_v8_context;

    uint32_t isolate_handle;

    v8::Persistent<v8::Module> *persistent;
    int hash;

    zval this_ptr;
    zend_object std;
};

PHP_MINIT_FUNCTION(php_v8_module);

#endif //PHP_V8_MODULE_H
