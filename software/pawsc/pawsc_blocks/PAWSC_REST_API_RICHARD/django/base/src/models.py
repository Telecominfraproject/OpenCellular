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


class Document(models.Model):
    description = models.CharField(max_length=255, blank=True)
    document = models.FileField(upload_to='documents/')
    uploaded_at = models.DateTimeField(auto_now_add=True)

class Notifyspectrumusedbmdata(models.Model):
    created_at = models.DateTimeField(blank=True, null=True)
    location_id = models.IntegerField(blank=True, null=True)
    config_id = models.IntegerField(blank=True, null=True)
    min = models.IntegerField(blank=True, null=True)
    max = models.IntegerField(blank=True, null=True)
    med = models.IntegerField(blank=True, null=True)
    nsteps = models.IntegerField(blank=True, null=True)
    v000 = models.IntegerField(blank=True, null=True)
    v001 = models.IntegerField(blank=True, null=True)
    v002 = models.IntegerField(blank=True, null=True)
    v003 = models.IntegerField(blank=True, null=True)
                  
    class Meta:
        managed = False
        db_table = 'notifySpectrumUsedBmData' 
#<insert v004 ... v127>
#https://stackoverflow.com/questions/24725724/django-creating-model-fields-from-list-with-for-loop

    pass
pref_dbm="v"
for i in range(4, 128): # range(i, n) implies from i to n-1
    column=str(i).zfill(3) #increment in pattern 001, 002, ..., **n
    field = pref_dbm+column #dynamically define field name i.e. v000, v001, v002, ..., v**n    
    Notifyspectrumusedbmdata.add_to_class(field, models.IntegerField(blank=True, null=True))
    #notifySpectrumUsedBmData.add_to_class(field, models.IntegerField(blank=True, null=True))
          

class Notifyspectrumuseconfig(models.Model):
    created_at = models.DateTimeField(blank=True, null=True)
    campaign_id = models.IntegerField(blank=True, null=True)
    start_freq = models.IntegerField(blank=True, null=True)
    end_freq = models.IntegerField(blank=True, null=True)
    amp_top = models.IntegerField(blank=True, null=True)
    amp_bottom = models.IntegerField(blank=True, null=True)
    nsteps = models.IntegerField(blank=True, null=True)
    f000 = models.FloatField(blank=True, null=True)
    f001 = models.FloatField(blank=True, null=True)
    f002 = models.FloatField(blank=True, null=True)
    f003 = models.FloatField(blank=True, null=True)
                 
    class Meta:
        managed = False
        db_table = 'notifySpectrumUseConfig'

 #<insert f004 ... f127>
    pass
pref_dbm="f"
for i in range(4, 128):
    column=str(i).zfill(3) #increment in pattern 001, 002, ..., **n
    field = pref_dbm+column #dynamically define field name i.e. v000, v001, v002, ..., v**n    
    Notifyspectrumuseconfig.add_to_class(field, models.FloatField(blank=True, null=True))
  