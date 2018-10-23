modprobe i2c-i801
i2cdetect -l

  
x=1
while [ $x -le 10 ]
do

#Temperature sensor(0X4C) : o/p-> ( Device id: 0x3d : 0x81) (manufacture id: 0x3e : 0x41) 

i2cdetect -y 9

echo "Checking presence of ADT7481 temperature sensor and DDR3 DIMM-0 SPD memory"

sleep 1

  x=$(( $x + 1 ))
done
