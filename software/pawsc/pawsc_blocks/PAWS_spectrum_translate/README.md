# Code to test translation from TVWS to LTE channel widths

## To install

Run the following prerequisites on mac osx

```
brew install curl-openssl
```
```
echo 'export PATH="/usr/local/opt/curl-openssl/bin:$PATH"' >> ~/.bash_profile
```
```
export PATH="/usr/local/opt/curl-openssl/bin:$PATH"
```
```
PYCURL_SSL_LIBRARY=openssl LDFLAGS="-L/usr/local/opt/openssl/lib" CPPFLAGS="-I/usr/local/opt/openssl/include" pip install --no-cache-dir pycurl
```

## To run

Use the following command

```
python query_paws.py <WiFi Channel width> <latitude> <longitude>
```
