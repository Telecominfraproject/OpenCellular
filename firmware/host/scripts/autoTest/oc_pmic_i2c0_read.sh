modprobe i2c-i801
#i2cdetect -l
#i2cdetect -r -y 9
#i2cdetect -r -y 0

#Valid only for ubuntu-14.04.4 : Kernel-> 4.2.0-42
bus=`i2cdetect -l | awk -F ' |-|\t' '/DesignWare/ {print $2}' | head -1`

i2cdetect -r -y $bus


  
# Read ‘Chip revision register’ from PMIC (0x5E) on i2c-0
# Sleep for a second

#PMIC(0X5E) o/p- chip revision: 0x05
x=1
while [ $x -le 100 ]
do

  
#i2cget -f -y 0 0x5e  0x01 b

i2cget -f -y $bus 0x5e  0x01 b  
echo "Reading Chip revision register’ from PMIC (0x5E) on i2c-0 and sleeping  for 1 sec"

sleep 1
  x=$(( $x + 1 ))
done
