<?php

// require_once $_SERVER['DOCUMENT_ROOT'] . DIRECTORY_SEPARATOR . 'config.php';
require_once dirname(__FILE__).'/config.php';

function get_conn(){
  $conn=mysql_connect(DB_HOST.':'.DB_PORT,DB_USER,DB_PWD);
  
  if(!$conn){
    logger('ERROR','DB: Can not get mysql connection');
    exit;
  }
  
  return $conn;
}


function select_db($conn){
  $db=mysql_select_db(DB_NAME, $conn);
  
  if(!$db){
    logger('ERROR', 'DB: Database does not exist');
    exit;
  }
  
  mysql_query("set names 'utf8'");
}


function connect_db(){
  $conn=get_conn();
  
  select_db($conn);
  
  return $conn;
}


/*unnecessary, also mysql_free_result($res)*/
function close($conn){
  mysql_close($conn);
}

?>
