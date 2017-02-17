--TEST--
V8\ScriptCompiler::CompileModule
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


$helper->header('Compiling');

$origin = new \V8\ScriptOrigin('test-module.js', 0, 0, false, 0, "", false, false, true);

$source_string = new V8\StringValue($isolate, '"test"');
$source = new \V8\ScriptCompiler\Source($source_string, $origin);
$module = V8\ScriptCompiler::CompileModule($context, $source);
$helper->assert('Compile module', $module instanceof \V8\Module);

$source_string = new V8\StringValue($isolate, 'var i = 0; while (true) {i++;}');
$source = new \V8\ScriptCompiler\Source($source_string, $origin);
$module = V8\ScriptCompiler::CompileModule($context, $source);
$helper->assert('Compile module', $module instanceof \V8\Module);

try {
    $source_string = new V8\StringValue($isolate, 'garbage garbage garbage');
    $source = new \V8\ScriptCompiler\Source($source_string, $origin);
    $module = V8\ScriptCompiler::CompileModule($context, $source);
} catch (\V8\Exceptions\TryCatchException $e) {
    $helper->exception_export($e);
    //$helper->dump($e->GetTryCatch());
}

try {
    $source_string = new V8\StringValue($isolate, 'does not matter');
    $source = new \V8\ScriptCompiler\Source($source_string);
    $module = V8\ScriptCompiler::CompileModule($context, $source);
} catch (\V8\Exceptions\GenericException $e) {
    $helper->exception_export($e);
}

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
