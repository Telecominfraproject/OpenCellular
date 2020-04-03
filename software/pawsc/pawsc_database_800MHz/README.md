# PAWSC for 800MHZ Cellular operating in TV White Space
This code is written in Python Django using the Django RESTAPI framework
https://www.django-rest-framework.org/

Documentation for this system is available here:
https://docs.google.com/document/d/1InU49wASjVizp_W0O-6CYSsXMDzp56q8qf-MD2glrRQ/edit?usp=sharing

# Instructions to start the server
Run the following command from the root folder

```
python3 manage.py runserver 0.0.0.0:8001
```

# To access the server to test the API
Open a web broswer and use the following URL
```
http://Server IP:8001/api/pawsc
```

# An example to request spectrum
Enter the following as a POST int he web interface
```
{"jsonrpc":"2.0", "method":"spectrum.pawsc.getSpectrum", "params":{"type":"AVAIL_SPECTRUM_REQ", "version":"1.0", "deviceDesc":{"serialNumber":"SN504", "fccId":"TA-2016/001","ModelID":"MN502"},"location":{"point":{"center":{"latitude":-34.129,"longitude":18.380}}}, "antenna":{"height":10.2,"heightType":"AGL"} }, "id":"103"}
```
