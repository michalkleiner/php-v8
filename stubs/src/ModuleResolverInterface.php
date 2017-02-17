<?php

namespace V8;

//typedef MaybeLocal<Module> (*ResolveCallback)(Local<Context> context, Local<String> specifier, Local<Module> referrer);
interface ModuleResolverInterface
{
    public function resolve(Context $context, StringValue $specifier, Module $referrer): Module;
}
