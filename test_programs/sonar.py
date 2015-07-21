
from scl import SCL_Reader, scl_get_socket
from time import sleep
from msgpack import loads

sonar = SCL_Reader('sonar_raw', 'sub')

sleep(3)
while True:
   
   print sonar[0], sonar[1], sonar[2], sonar[3]
