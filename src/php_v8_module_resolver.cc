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

#include "php_v8_module_resolver.h"
#include "php_v8.h"

zend_class_entry* php_v8_module_resolver_class_entry;
#define this_ce php_v8_module_resolver_class_entry


PHP_V8_ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_v8_module_resolver_resolve, ZEND_RETURN_VALUE, 3, V8\\Module, 0)
                ZEND_ARG_OBJ_INFO(0, context, V8\\Context, 0)
                ZEND_ARG_OBJ_INFO(0, specifier, V8\\StringValue, 0)
                ZEND_ARG_OBJ_INFO(0, referrer, V8\\Module, 0)
ZEND_END_ARG_INFO()


static const zend_function_entry php_v8_module_resolver_methods[] = {
    PHP_ABSTRACT_ME(V8Source, resolve,  arginfo_v8_module_resolver_resolve)

    PHP_FE_END
};


PHP_MINIT_FUNCTION(php_v8_module_resolver)
{
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, PHP_V8_NS, "ModuleResolverInterface", php_v8_module_resolver_methods);
    this_ce = zend_register_internal_interface(&ce);

    return SUCCESS;
}
