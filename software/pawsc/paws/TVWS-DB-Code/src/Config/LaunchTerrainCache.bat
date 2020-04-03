:: Copyright (c) Microsoft Corporation. All rights reserved.
:: Licensed under the MIT License.



Path=C:\Program Files\Microsoft SDKs\Windows Azure\Emulator;C:\Program Files\Microsoft SDKs\Windows Azure\.NET SDK\v2.2\bin
cd\
e:
cd E:\Projects\Office\Whitespace\RegionSyncService
cspack E:\Projects\Office\Whitespace\RegionSyncService\ServiceDefinition.csdef /copyonly /role:TerrianCache;E:\Projects\Office\Whitespace\TerrianCache\bin\Debug;Microsoft.Whitespace.TerrianCache.dll

cd E:\Projects\Office\Whitespace\RegionSyncService
csrun ServiceDefinition.csx ServiceConfiguration.Local.cscfg

pause
