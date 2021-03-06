--TEST--
Object update
--FILE--
<?php

include_once "connect.inc";

$key    = "my_key_update";
$client = new \Riak\Connection($host, $port);
$object = new \Riak\Object($key);
$props  = new \Riak\BucketPropertyList($nVal = 1, $allowMult = true);
$bucket = new \Riak\Bucket($client, 'test_bucket');

$objectValue1 = "object value 1";
$objectValue2 = "object value 2";

$bucket->setPropertyList($props);
$object->setContent($objectValue1);
$bucket->put($object);

var_dump($bucket->get($key)->hasSiblings());
var_dump($bucket->get($key)->getFirstObject()->getContent());

$output = $bucket->get($key);
$object = $output->getFirstObject();

var_dump($object->getVClock() === $output->getVClock());

$object->setContent($objectValue2);
$bucket->put($object);

$getOutput = $bucket->get($key);
var_dump($getOutput->getFirstObject()->getContent());
var_dump($getOutput->hasSiblings());

$deleteInput = new \Riak\Input\DeleteInput();
$bucket->delete($key, $deleteInput->setVClock($getOutput->getVClock()));
?>
--EXPECT--
bool(false)
string(14) "object value 1"
bool(true)
string(14) "object value 2"
bool(false)
