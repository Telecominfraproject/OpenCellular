# In dora a SRC_URI = "file://*.service" is done and that doesn't work
# when playing with the FILESPATH
SRC_URI = "\
	file://machineid.service \
	file://run-postinsts.service \
	"
