require libspatialite.inc

PR = "${INC_PR}.0"

SRC_URI[md5sum] = "83305ed694a77152120d1f74c5151779"
SRC_URI[sha256sum] = "9f138a6854740c7827fdee53845eb1485fce3e805a7aa9fc9151f8046ebd312d"

SRC_URI += "file://geos-config.patch"
