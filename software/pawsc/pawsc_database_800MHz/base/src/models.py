from django.db import models

# Create your models here.

class Device_Description(models.Model):
    manufacturer = models.CharField(max_length=50)
    modelID = models.CharField(max_length=50)


class Ellipse(models.Model):
    centerLatitude =  models.FloatField()
    centerLongitude = models.FloatField()
    semiMajorAxis = models.FloatField(null=True, blank=True)
    semiMinorAxis = models.FloatField(null=True, blank=True)
    orientation = models.FloatField(null=True, blank=True)


class Geolocation(models.Model):
    point = models.ForeignKey(Ellipse, on_delete=models.CASCADE, null=True, blank=True)
    confidence = models.IntegerField(default=0)

class Polygon(models.Model):
    centerLatitude =  models.FloatField()
    centerLongitude = models.FloatField()
    regions = models.ForeignKey(Geolocation, on_delete=models.CASCADE)

class Scan_Device(models.Model):
    deviceID = models.ForeignKey(Device_Description, on_delete=models.CASCADE, null=True, blank=True)
    serialNumber = models.CharField(max_length=50)
    location = models.ForeignKey(Geolocation, on_delete=models.CASCADE, null=True, blank=True)


class Antenna_Description(models.Model):
    manufacturer = models.CharField(max_length=50)
    modelID = models.CharField(max_length=50)
    maxGain = models.IntegerField(default=0)

class Antenna_List(models.Model):
    scanDevice  = models.ForeignKey(Scan_Device, on_delete=models.CASCADE)
    antennaElevation = models.IntegerField(default=0)
    antennaAzimuth = models.IntegerField(default=0)
    antennaCableLoss = models.IntegerField(default=0)
    antenna = models.ForeignKey(Antenna_Description, on_delete=models.CASCADE, null=True, blank=True)


