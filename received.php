<?php
	$row = $_GET["data"];

	$date = date('Y-m-d');
	$output = "";
	$myFile = "test.log";
	if (!file_exists($myFile)) {
		echo 'File not found';
	}
	$fh = fopen($myFile, 'a') or die("couldn't open");
	$output .= date('H:i:s');
	$output .= " - ";
	$output .= $row;
	$output .= "\n";
	fwrite($fh, $output );
	fclose($fh);
	

?>