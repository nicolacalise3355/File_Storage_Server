//pseudocodice funzione addizione php
function getSomma($arrayParams){
	$tmp = 0;
	for($i = 0; $i < count($arrayParams); $i++){
		$tmp = $tmp + $arrayParams[$i];
	}
	return $tmp;
}