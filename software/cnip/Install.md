# Setting up CNIP on Windows Server

- Enable Windows IIS feature

- Install [Visual Studio](https://visualstudio.microsoft.com/downloads/) 2017 or above

- Install [PostgreSQL](https://www.postgresql.org/) and [PostGIS](https://postgis.net/)

- Unzip the [SPLAT](SPLAT.7z) installation to C:/SPLAT

- Create a database 'cnip' on PostgreSQL

- Execute [databaseSetup.sql](databaseSetup.sql) script on 'cnip' database

- Download elevation data from below links and convert to sdf format, please read the [SPLAT](https://www.qsl.net/kd2bd/splat.html) documentation 
	- [https://e4ftl01.cr.usgs.gov/MEASURES/SRTMGL1.003/2000.02.11/](https://e4ftl01.cr.usgs.gov/MEASURES/SRTMGL1.003/2000.02.11/)
	- [https://e4ftl01.cr.usgs.gov/MEASURES/SRTMGL3.003/2000.02.11/](https://e4ftl01.cr.usgs.gov/MEASURES/SRTMGL3.003/2000.02.11/)

- Fork ['cnip'](cnip) and ['forecast'](forecast) solutions to C:/TelecomInfraProject/OpenCellular/

- Open both solutions, goto NuGet and restore missing packages from online sources

- Build 'forecast' first then Build and Publish 'cnip' solution to localhost using custom profile already setup

- You need to add .az & .ez antenna pattern files required by SPLAT in C:/SPLAT/antenna folder and enable the commented code in coverage.cs file in 'forecast' solution

End