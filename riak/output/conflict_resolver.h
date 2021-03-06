/*
   Copyright 2013: Kaspar Bach Pedersen

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

#ifndef RIAK_OUTPUT__CONFLICT_RESOLVER__H__
#define RIAK_OUTPUT__CONFLICT_RESOLVER__H__

#include "php_riak_internal.h"


#define RIAK_OUTPUT_CONFLICT_RESOLVER_ARG_INFO_EXEC(name) \
    ZEND_BEGIN_ARG_INFO_EX(name, 0, ZEND_RETURN_VALUE, 1) \
       ZEND_ARG_OBJ_INFO(0, objects, Riak\\ObjectList, 0) \
   ZEND_END_ARG_INFO();

PHP_METHOD(Riak_Output_ConflictResolver, resolve);

extern zend_class_entry *riak_output_conflict_resolver_ce;

void riak_output_conflict_resolver_init(TSRMLS_D);

#endif //RIAK_OUTPUT__CONFLICT_RESOLVER__H__
