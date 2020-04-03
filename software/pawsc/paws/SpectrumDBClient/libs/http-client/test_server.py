##
## Copyright (c) Microsoft Corporation. All rights reserved.
## Licensed under the MIT License.
##

from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer


class S(BaseHTTPRequestHandler):
    def _set_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    def do_POST(self):
        self._set_headers()
        print "In post method"
        print self.rfile.read(int(self.headers['Content-Length']))
        self.send_response(200)
        self.end_headers()


server_address = ('', 80)
httpd = HTTPServer(server_address, S)
print 'Starting httpd...'
httpd.serve_forever()
