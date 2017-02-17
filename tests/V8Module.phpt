--TEST--
V8\Module
--SKIPIF--
<?php if (!extension_loaded("v8")) print "skip"; ?>
--FILE--
<?php

/** @var \Phpv8Testsuite $helper */
$helper = require '.testsuite.php';

require '.v8-helpers.php';
$v8_helper = new PhpV8Helpers($helper);


$isolate = new V8\Isolate();
$context = new V8\Context($isolate);

//$context->GlobalObject()->Set($context, new \V8\StringValue($isolate, 'print'), $v8_helper->getPrintFunctionTemplate($isolate)->GetFunction($context));
$v8_helper->injectConsoleLog($context);

$helper->header('Compiling');

$origin = new \V8\ScriptOrigin('test-module.js', 0, 0, false, 0, "", false, false, true);


$source_string = new V8\StringValue($isolate, 'import * as test from "test-module-inner.js"; console.log(test)');
//$source_string = new V8\StringValue($isolate, 'import * as test from "test-module-inner.js"; "test"');
//$source_string = new V8\StringValue($isolate, 'console.log("Test")');
$source = new \V8\ScriptCompiler\Source($source_string, $origin);
$module = V8\ScriptCompiler::CompileModule($context, $source);
$helper->assert('Compile module', $module instanceof \V8\Module);

class TestResolver implements \V8\ModuleResolverInterface {
    /**
     * @var Phpv8Testsuite
     */
    private $helper;

    /**
     * @param \Phpv8Testsuite $helper
     */
    public function __construct($helper)
    {
        $this->cnt = 0;
        $this->helper = $helper;
    }
    public function resolve(\V8\Context $context, \V8\StringValue $specifier, \V8\Module $referrer): \V8\Module
    {
        $this->cnt++;
        $this->helper->inline("Resolving module {$this->cnt}", $specifier->Value());
        //var_dump(func_get_args(), $specifier->Value());
        // TODO: Implement resolve() method.

        //$other_context = new \V8\Context($context->GetIsolate());
        $other_context = $context;

        $origin = new \V8\ScriptOrigin('test-module-inner.js', 0, 0, false, 0, "", false, false, true);

        $source_string = new V8\StringValue($context->GetIsolate(), '
        //import "another.js";
        console.log("inner 1");
        export const foo = "bar";
        console.log("inner 2");
        ');
        $source = new \V8\ScriptCompiler\Source($source_string, $origin);
        $module = V8\ScriptCompiler::CompileModule($context, $source);
        $this->helper->assert('Compile nested module', $module instanceof \V8\Module);

        $module->Instantiate($context, $this);
        return $module;
    }
};

$resolver = new TestResolver($helper);

$status = $module->Instantiate($context, $resolver);

$helper->assert('Module instantiated', $status === true);

$res = $module->Evaluate($context);

//$helper->dump($res);
$helper->dump($res->Value());

$helper->space();


$helper->header('Testing');



?>
--EXPECT--
Compiling:
----------
Compile module: ok
Compile module: ok
V8\Exceptions\TryCatchException: SyntaxError: Unexpected identifier
V8\Exceptions\GenericException: Unable to compile non-module as module

Testing:
--------
