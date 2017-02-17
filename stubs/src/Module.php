<?php

namespace V8;


/**
 * This is an unfinished experimental feature, and is only exposed
 * here for internal testing purposes. DO NOT USE.
 *
 * A compiled JavaScript module.
 */
class Module
{

    private function __construct()
    {
    }

    /**
     * Returns array of modules specifier requested by this module.
     *
     * This method combine original v8::Module::GetModuleRequestsLength() and v8::Module::GetModuleRequestsLength()
     * into a single method to better match PHP real-life usage
     *
     * @return StringValue[]
     */
    public function GetModuleRequests(): array
    {
    }

    /**
     * Returns the identity hash for this object.
     */
    public function GetIdentityHash(): int
    {
    }

    /**
     * ModuleDeclarationInstantiation
     *
     * Returns false if an exception occurred during instantiation.
     *
     * @param Context                 $context
     * @param ModuleResolverInterface $resolver
     *
     * @return bool
     */
    public function Instantiate(Context $context, ModuleResolverInterface $resolver): bool
    {
    }

    /**
     * ModuleEvaluation
     *
     * @param Context $context
     *
     * @return Value
     */
    public function Evaluate(Context $context): Value
    {
    }
}
