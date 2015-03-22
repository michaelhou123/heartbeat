--DB
create database db_heartbeat;
use db_heartbeat;

drop table if exists tb_heartbeat;
create table tb_heartbeat (
  id             int unsigned  not null auto_increment,
  host           varchar(15)   default '127.0.0.1',
  service        varchar(32)   default null,
  component      varchar(96)   default null,
  beat_interval  int unsigned  default 0 comment 'seconds',
  beat_time      datetime      default null,
  primary key (id),
  key idx_service_component (service,component)
) engine=innodb;

insert into tb_heartbeat (service,component,beat_interval,beat_time) values('XService','XComponent',60,now());
insert into tb_heartbeat (service,component,beat_interval,beat_time) values('XWeb','Tomcat OOM',120,now());
insert into tb_heartbeat (service,component,beat_interval,beat_time) values('System','Disk inode almost full',300,now());
insert into tb_heartbeat (service,component,beat_interval,beat_time) values('DB','Slave replication delay',3600,now());
insert into tb_heartbeat (service,component,beat_interval,beat_time) values('Business','Download number',86400,now());
update tb_heartbeat set beat_time=now() where host='127.0.0.1' and service='TestService' and component='TestComponent';
