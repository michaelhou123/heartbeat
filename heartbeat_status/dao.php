<?php
require_once dirname(__FILE__).'/'.'logger.php';
require_once dirname(__FILE__).'/'.'mysql.php';


function get_heartbeat_list(){
  $conn=connect_db();
  $sql='select host,service,component,beat_interval,beat_time from tb_heartbeat order by host';
  $res=mysql_query($sql,$conn);
  
  $heartbeat_list=array();
  $item=array();
  if($res){
    while($row=mysql_fetch_array($res)){
      $item['host']=$row['host'];
      $item['service']=$row['service'];
      $item['component']=$row['component'];
      $item['beat_interval']=$row['beat_interval'];
      $item['beat_time']=$row['beat_time'];
      $item['status']=(time()-strtotime($row['beat_time']))<$item['beat_interval']?true:false;
      $item['duration']=round((time()-strtotime($row['beat_time']))/60,0);
      array_push($heartbeat_list,$item);
    }
  }
  
  return $heartbeat_list;
}

?>
