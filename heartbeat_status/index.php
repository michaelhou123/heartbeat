<?php
require_once dirname(__FILE__).'/'.'dao.php';

$heartbeat_list=get_heartbeat_list();

?>

<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<title>Heartbeat Status</title>
<link rel="stylesheet" type="text/css" href="./basic.css" />
</head>
<body>

<div id="top_area">
  <div id="logo">
    Heartbeat Status
  </div>
  <div id="link">
    <a href="#" target="_blank" style="color: #FFF;">CMS</a>
  </div>
  <div class="clear"></div>
</div>

<div class="div_block">
  <table>
    <tr class="table-head">
      <th>Seq.</th>
      <th>Host</th>
      <th>Service</th>
      <th>Component</th>
      <th>Beat Interval</th>
      <th>Beat Time</th>
      <th>Duration</th>
    </tr>
    <?php
    $i=1;
    foreach($heartbeat_list as $item){
      if(!$item['status']){ ?>
    <tr class="<?php echo $i%2==0?'even-color':''; ?>">
      <td><?php echo $i++; ?></td>
      <td><?php echo $item['host']; ?></td>
      <td><?php echo $item['service']; ?></td>
      <td><?php echo $item['component']; ?></td>
      <td><?php echo $item['beat_interval']; ?>s</td>
      <td>
        <span style="color:#E06043;">
          <?php echo $item['beat_time'];?>
        <span>
      </td>
      <td><?php echo $item['duration'];?>m</td>
    </tr>
    <?php }} ?>
  </table>
</div>

<div class="div_block">
  <table>
    <tr class="table-head">
      <th>Seq.</th>
      <th>Host</th>
      <th>Service</th>
      <th>Component</th>
      <th>Beat Interval</th>
      <th>Beat Time</th>
    </tr>
    <?php $i=1;foreach($heartbeat_list as $item){if($item['status']){ ?>
    <tr class="<?php echo $i%2==0?'even-color':''; ?>">
      <td><?php echo $i++; ?></td>
      <td><?php echo $item['host']; ?></td>
      <td><?php echo $item['service']; ?></td>
      <td><?php echo $item['component']; ?></td>
      <td><?php echo $item['beat_interval']; ?>s</td>
      <td>
        <span style="color:#095720;">
          <?php echo $item['beat_time'];?>
        <span>
      </td>
    </tr>
    <?php }} ?>
  </table>
</div>

</body>
</html>
