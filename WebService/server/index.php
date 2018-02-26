<?php

	$dir = "bin\\";
	$target_dir = $dir;
	$uploadOk = 1;
	$target_file = $target_dir . basename($_FILES["fileToUpload"]["name"]);
	if (move_uploaded_file($_FILES["fileToUpload"]["tmp_name"], $target_file)) 
	{
		//echo "File uploaded. Conversion in progress..."."<br>";
		$uploadOk = 1;
	}
	else{
		  echo "File move on upload failed"."<br>";
		$uploadOk = 0;
	}

	if ($uploadOk == 1)
	{
		$binApp = $dir."App.exe";
	
		$callString = $binApp.' '.$target_file ;
		
		//echo ($callString."<br>");
	
		exec ($callString, $output, $retvar);
	
		if ($retvar == 1)
		{
			//echo "File convertion successful"."<br>";
			$fileToSend = $target_file . ".html";
			//header("Content-disposition: attachment;filename=$fileToSend");
			readfile($fileToSend);	
			unlink($fileToSend);
		}
		else{
			echo "File convertion failed"."<br>";
		}
		unlink($target_file);
	}
?> 

