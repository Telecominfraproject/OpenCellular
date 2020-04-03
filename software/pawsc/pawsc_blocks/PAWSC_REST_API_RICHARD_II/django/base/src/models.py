from django.db import models

# Create your models here.

''' 
 These  are auto-generated Django model modules.
 You'll have to do the following manually to clean this up:
   * Rearrange models' order
   * Make sure each model has one field with primary_key=True
   * Make sure each ForeignKey has `on_delete` set to the desired behavior.
   * Remove `managed = False` lines if you wish to allow Django to create, modify, and delete the table
 Feel free to rename the models, but don't rename db_table values or field names.
'''
class RegisteredDevices(models.Model):
    serial_number = models.TextField(blank=True, null=True)
    latitude = models.FloatField(blank=True, null=True)
    longitude = models.FloatField(blank=True, null=True)
    antenna_height = models.FloatField(blank=True, null=True)
    antenna_type = models.TextField(blank=True, null=True)  
    date = models.TextField(blank=True, null=True)

    class Meta:
        managed = False
        db_table = 'registered_devices'

class InitialisedDevices(models.Model):
    serial_number = models.TextField(blank=True, null=True)
    latitude = models.FloatField(blank=True, null=True)
    longitude = models.FloatField(blank=True, null=True)
    antenna_height = models.FloatField(blank=True, null=True)
    antenna_type = models.TextField(blank=True, null=True)
    device_type = models.TextField(blank=True, null=True)
    device_capabilities = models.TextField(blank=True, null=True)
    device_description = models.TextField(blank=True, null=True)
    time = models.TextField(blank=True, null=True)

    class Meta:
        managed = False
        db_table = 'initialised_devices'

class DynamicFreqAssignment(models.Model):
    id = models.TextField(db_column='ID', primary_key=True)  # Field name made lowercase. This field type is a guess.
    license_id = models.IntegerField(db_column='license_ID')  # Field name made lowercase.
    freqstart = models.IntegerField(db_column='freqStart')  # Field name made lowercase.
    freqend = models.IntegerField(db_column='freqEnd')  # Field name made lowercase.

    class Meta:
        managed = False
        db_table = 'dynamic_freq_assignment' 


class UnassignedFreq(models.Model):
    id = models.TextField(db_column='ID', primary_key=True)  # Field name made lowercase. This field type is a guess.
    freqstart = models.IntegerField(db_column='freqStart')  # Field name made lowercase.
    freqend = models.IntegerField(db_column='freqEnd')  # Field name made lowercase.
    band = models.IntegerField(db_column='band')

    class Meta:
        managed = False
        db_table = 'unassigned_freq'