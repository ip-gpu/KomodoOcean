<?php

// Simple PHP script to get *.cmd batch files from assetchains.old

if ($file = fopen("assetchains.old", "r")) {
while(!feof($file)) {
$line = fgets($file);
preg_match('/-ac_name=(\w+)/', $line, $matches);

if (count($matches) > 0) {

$line = str_replace("./komodod ","start ..\\KomodoOceanGUI.exe ",$line);
$line = str_replace("&","",$line);
$line = str_replace("$1","",$line);
$line = str_replace("-pubkey=\$pubkey ","",$line);
$line = trim($line);
file_put_contents(strtolower($matches[1]).".cmd", $line);

}

}}

?>