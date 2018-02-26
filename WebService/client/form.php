<?php
	$url = "http://160.40.50.183:80/P4All/certhOCR/";
	
	$curl = curl_init();
    curl_setopt($curl, CURLOPT_POST, 1);


	//backwards compatible version, modern version use curlFile instead of @
	$data = array( 'fileToUpload' => '@I:\\DATA\\ocr\\001.png');
	curl_setopt($curl, CURLOPT_SAFE_UPLOAD, false);
	
	curl_setopt($curl, CURLOPT_POSTFIELDS, $data);
	curl_setopt($curl, CURLOPT_URL, $url);
    curl_setopt($curl, CURLOPT_RETURNTRANSFER, 1);

    $result = curl_exec($curl);

    curl_close($curl);

    echo $result;
?> 

