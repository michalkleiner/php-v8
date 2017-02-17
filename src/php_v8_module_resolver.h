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

#ifndef PHP_V8_MODULE_RESOLVER_H
#define PHP_V8_MODULE_RESOLVER_H

extern "C" {
#include "php.h"

#ifdef ZTS
#include "TSRM.h"
#endif
}

extern zend_class_entry *php_v8_module_resolver_class_entry;


PHP_MINIT_FUNCTION(php_v8_module_resolver);

#endif //PHP_V8_MODULE_RESOLVER_H
