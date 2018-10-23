modprobe i2c-i801
i2cdetect -l
i2cdetect -r -y 9
#i2cdetect -r -y 0

  

x=1
while [ $x -le 100 ]
do

#Read device register address from 128 to 145 from device DDR3 SPD (0x50) on i2c-9

y=128

while [ $y -le 145 ]
do
i2cget -f -y 9 0x50   $y  b
y=$(( $y + 1 ))
done

echo "Reading device register address from 128 to 145 from device DDR3 SPD (0x50) on i2c-9 and sleeping for 1 sec "

sleep 1

  x=$(( $x + 1 ))
done
