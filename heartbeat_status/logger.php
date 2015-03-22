<?php
$logfile='/tmp/heartbeat_status.log';

/*Format: Time -- IP -- Level -- Message*/
function logger($level, $msg){
  $f=fopen($GLOBALS['logfile'],'a+');
  
  $datetime=date('Y-m-d H:i:s O');
  
  fputs($f,$datetime.' -- '.$_SERVER['REMOTE_ADDR'].' -- '.$level.' -- '.$msg."\n");
  
  fclose($f);
}

?>
