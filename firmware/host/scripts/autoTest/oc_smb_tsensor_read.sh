modprobe i2c-i801
i2cdetect -l
i2cdetect -r -y 9
#i2cdetect -r -y 0

  
x=1
while [ $x -le 100 ]
do

#Temperature sensor(0X4C) : o/p-> ( Device id: 0x3d : 0x81) (manufacture id: 0x3e : 0x41) 

i2cget -f -y 9 0x1f   0x3d b

echo "Reading -> Device id: 0x3d : 0x81 from temperature sensor and sleeping  for 1 sec"

sleep 1

i2cget -f -y 9 0x1f   0x3e b  

echo "Reading -> Manufacture id: 0x3e : 0x41 from temperature sensor and sleeping  for 1 sec"

sleep 1
  x=$(( $x + 1 ))
done
